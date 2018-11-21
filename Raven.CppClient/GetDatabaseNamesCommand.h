#pragma once
#include "stdafx.h"
#include "RavenCommand.h"

namespace ravendb::client::serverwide::operations
{
	namespace
	{
		struct DatabaseNames_t//used with to/from json
		{
			std::vector<std::string> value;
		};

		void from_json(const nlohmann::json& j, DatabaseNames_t& result)
		{
			auto&& res = result.value;
			auto&& it = j.find("Databases");
			if (it == j.end() || ! it->is_array())
			{
				throw RavenError({}, RavenError::ErrorType::invalid_response);
			}
			else
			{
				res = it->get<std::decay_t<decltype(res)>>();
			}
		}
	}

	class GetDatabaseNamesCommand : public RavenCommand<std::vector<std::string>>
	{
	private:
		int32_t _start{};
		int32_t _page_size{};

	public:

		~GetDatabaseNamesCommand() override = default;

		GetDatabaseNamesCommand(int32_t _start, int32_t _page_size)
			: _start(_start),
			  _page_size(_page_size)
		{}

		void create_request(CURL* curl, const ServerNode& node, std::string& url) const override
		{
			std::ostringstream pathBuilder;
			pathBuilder << node.url << "/databases?start=" << _start
				<< "&pageSize=" << _page_size << "&namesOnly=true";

			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

			url = pathBuilder.str();
		}


		void set_response(CURL* curl, const nlohmann::json& response, bool from_cache) override
		{
			DatabaseNames_t res = response;
			_result = std::move(res.value);
		}

		bool is_read_request() const noexcept override
		{
			return true;
		}
	};
}