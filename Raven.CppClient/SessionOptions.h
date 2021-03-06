#pragma once
#include "TransactionMode.h"
#include "RequestExecutor.h"

namespace ravendb::client::documents::session
{
	struct SessionOptions
	{
		std::string database{};
		bool no_tracking = false;
		bool no_caching = false;
		std::shared_ptr<http::RequestExecutor> request_executor{};
		TransactionMode transaction_mode = TransactionMode::UNSET;
	};
}
