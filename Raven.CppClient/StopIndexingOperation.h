#pragma once
#include "stdafx.h"
#include "IVoidMaintenanceOperation.h"

using
ravendb::client::http::RavenCommand,
ravendb::client::http::ServerNode,
ravendb::client::http::VoidRavenCommand;

namespace ravendb::client::documents::operations::indexes
{
	class StopIndexingOperation : public IVoidMaintenanceOperation
	{
	public:
		~StopIndexingOperation() override = default;

		StopIndexingOperation() = default;

		std::unique_ptr<VoidRavenCommandBase> get_command(const DocumentConventions& conventions) const override
		{
			return std::make_unique<StopIndexingCommand>();
		}

	private:
		class StopIndexingCommand : public VoidRavenCommand
		{
		public:
			~StopIndexingCommand() override = default;

			StopIndexingCommand() = default;

			void create_request(CURL* curl, const ServerNode& node, std::string& url) override
			{
				std::ostringstream pathBuilder;
				pathBuilder << node.url << "/databases/" << node.database
					<< "/admin/indexes/stop";

				curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
				curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, "");

				url = pathBuilder.str();
			}
		};
	};
}

