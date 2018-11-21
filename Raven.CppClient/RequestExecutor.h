#pragma once

#include "stdafx.h"
#include "CurlHandlesHolder.h"
#include "Topology.h"
#include "RavenCommand.h"
#include "utils.h"

namespace ravendb::client::http
{
	class RequestExecutor
	{
	private:
		std::string _db_name;
		std::vector<std::string> _initial_urls;
		std::shared_ptr<Topology> _topology;
		std::string _certificate;
		std::unique_ptr<impl::CurlHandlesHolder> _curl_holder;
		impl::SetCurlOptions _set_before_perform = {};
		impl::SetCurlOptions _set_after_perform = {};

		void validate_urls();

		void first_topology_update();

		template<typename Result_t>
		Result_t execute_internal(ServerNode& node, RavenCommand<Result_t>& cmd)
		{
			char error_buffer[CURL_ERROR_SIZE] = { '\0' };
			std::string output_buffer;
			std::map<std::string, std::string> headers;
			auto ch = _curl_holder->checkout_curl();
			auto curl = ch.get();

			std::string url;
			cmd.create_request(curl, node, url);

			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ravendb::client::impl::utils::push_to_buffer);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_buffer);
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, ravendb::client::impl::utils::push_headers);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

			if (_set_before_perform)
			{
				_set_before_perform(curl);
			}
			auto res = curl_easy_perform(curl);

			long statusCode;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
			if (res != CURLE_OK)
			{
				std::ostringstream error_builder;
				error_builder << "Failed request to: "
					<< url
					<< ", status code: "
					<< std::to_string(statusCode)
					<< "\n"
					<< error_buffer
					<< "\n";

				throw RavenError(error_builder.str(), RavenError::ErrorType::generic);
			}
			if(_set_after_perform)
			{
				_set_after_perform(curl);
			}


			cmd.status_code = statusCode;
			switch (statusCode)
			{
				case 200:
				case 201:
				{
					auto result = nlohmann::json::parse(output_buffer);
					cmd.set_response(curl, result, false);
					break;
				}
				case 204:
					break;
				case 304:
					break;
				case 404:
					cmd.set_response_not_found(curl);
					break;
				case 503:
					if (headers.find("Database-Missing") != headers.end())
					{
						throw RavenError(output_buffer, RavenError::ErrorType::database_does_not_exist);
					}
					throw RavenError(output_buffer, RavenError::ErrorType::service_not_available);
				case 500:
					throw RavenError(output_buffer, RavenError::ErrorType::internal_server_error);
				default:
					throw RavenError(output_buffer, RavenError::ErrorType::unexpected_response);
			}

			return cmd.get_result();
		}

		RequestExecutor(
			std::string db_name,
			std::vector<std::string> initial_urls,
			std::string certificate,
			ravendb::client::impl::SetCurlOptions set_before_perform,
			ravendb::client::impl::SetCurlOptions set_after_perform);

	public:

		RequestExecutor(const RequestExecutor& other) = delete;
		RequestExecutor(RequestExecutor&& other) = delete;

		RequestExecutor& operator=(const RequestExecutor& other) = delete;
		RequestExecutor& operator=(RequestExecutor&& other) = delete;

		~RequestExecutor() = default;

		template<typename Result_t>
		Result_t execute(RavenCommand<Result_t>& cmd)
		{
			std::optional<std::ostringstream> errors;

			std::shared_ptr<Topology> topology_local = std::atomic_load(&_topology);

			for (auto& node : topology_local->nodes)
			{
				try
				{
					 return execute_internal(node, cmd);
				}
				catch (RavenError& re)
				{
					if(! errors)
					{
						errors.emplace();
					}
					errors.value() << re.what() << '\n';
					if (re.get_error_type() == RavenError::ErrorType::database_does_not_exist)
					{
						throw RavenError(errors->str(), RavenError::ErrorType::database_does_not_exist);
					}
					continue;
				}
			}
			throw RavenError(errors->str(), RavenError::ErrorType::generic);
		}


		static std::unique_ptr<RequestExecutor> create(
			std::vector<std::string> urls,
			std::string db,
			std::string certificate = {},
			ravendb::client::impl::SetCurlOptions set_before_perform = {},
			ravendb::client::impl::SetCurlOptions set_after_perform = {});
	};
}