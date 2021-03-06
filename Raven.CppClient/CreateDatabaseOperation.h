#pragma once
#include "IServerOperation.h"
#include "RavenCommand.h"
#include "DocumentConventions.h"
#include "DatabaseRecord.h"
#include "DatabasePutResult.h"

namespace ravendb::client
{
	namespace http
	{
		struct ServerNode;
	}
	namespace serverwide
	{
		struct DatabaseRecord;
	}
}

namespace ravendb::client::serverwide::operations
{
	class CreateDatabaseOperation : public IServerOperation<DatabasePutResult>
	{
	private:
		const DatabaseRecord _database_record;
		const int32_t _replication_factor;

	public:
		~CreateDatabaseOperation() override;

		CreateDatabaseOperation(DatabaseRecord database_record, int32_t replication_factor = 1);

		std::unique_ptr<http::RavenCommand<DatabasePutResult>> get_command(const documents::conventions::DocumentConventions& conventions) override;

	private:
		class CreateDatabaseCommand : public http::RavenCommand<DatabasePutResult>
		{
		private:
			const documents::conventions::DocumentConventions _conventions;
			const DatabaseRecord _database_record;
			const int32_t _replication_factor;
			const std::string _database_name;
			const std::string _database_document;

		public:

			~CreateDatabaseCommand() override;

			CreateDatabaseCommand(const documents::conventions::DocumentConventions& conventions,
				DatabaseRecord database_record, int32_t replication_factor);

			void create_request(CURL* curl, const http::ServerNode& node, std::string& url) override;

			void set_response(CURL* curl, const nlohmann::json& response, bool from_cache) override;

			bool is_read_request() const noexcept override;
		};
	};
}
