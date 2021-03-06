#pragma once
#include "HttpStatus.h"

namespace ravendb::client::impl::utils
{
	struct CompareStringsIgnoreCase;
}

namespace ravendb::client::documents::commands::multi_get
{
	struct GetResponse
	{
		std::optional<std::string> result{};
		std::map<std::string, std::string, impl::utils::CompareStringsIgnoreCase> headers{};
		int32_t status_code{};
		bool force_retry = false;

		bool request_has_errors() const
		{
			bool are_errors = false;

			switch (status_code)
			{
			case 0:
			case (int32_t)http::HttpStatus::SC_OK:
			case (int32_t)http::HttpStatus::SC_CREATED:
			case (int32_t)http::HttpStatus::SC_NON_AUTHORITATIVE_INFORMATION:
			case (int32_t)http::HttpStatus::SC_NO_CONTENT:
			case (int32_t)http::HttpStatus::SC_NOT_MODIFIED:
			case (int32_t)http::HttpStatus::SC_NOT_FOUND:
				break;
			default:
				are_errors = true;
				break;
			}
			return are_errors;
		}
	};

	inline void from_json(const nlohmann::json& j, GetResponse& gr)
	{
		using impl::utils::json_utils::get_val_from_json;

		get_val_from_json(j, "Headers", gr.headers);
		if(auto it = j.find("Result");
			it != j.end())
		{
			gr.result = it->dump();
		}
		get_val_from_json(j, "StatusCode", gr.status_code);
		get_val_from_json(j, "ForceRetry", gr.force_retry);
	}
}
