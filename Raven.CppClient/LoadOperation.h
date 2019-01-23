#pragma once
#include "InMemoryDocumentSessionOperations.h"
#include "GetDocumentsResult.h"
#include "GetDocumentsCommand.h"
#include "DocumentsByIdsMap.h"

namespace ravendb::client::documents::session::operations
{
	class LoadOperation
	{
	private:
		const std::reference_wrapper<InMemoryDocumentSessionOperations> _session;

		std::vector<std::string> _ids{};

		std::vector<std::string> _ids_to_check_on_server{};

		std::optional<commands::GetDocumentsResult> _current_load_results{};

	public:
		LoadOperation(InMemoryDocumentSessionOperations& session)
			: _session(std::ref(session))
		{}

		std::unique_ptr<RavenCommand<commands::GetDocumentsResult>> create_request() const
		{
			if(_ids_to_check_on_server.empty())
			{
				return {};
			}

			_session.get().increment_request_count();

			return std::make_unique<commands::GetDocumentsCommand>(_ids_to_check_on_server, std::vector<std::string>(), false);
		}

		LoadOperation& by_id(const std::string& id)
		{
			if(_ids.empty())
			{
				_ids.push_back(id);
			}
			if(_session.get().is_loaded_or_deleted(id))
			{
				return *this;
			}

			_ids_to_check_on_server.push_back(id);
			return *this;
		}

		LoadOperation& by_ids(const std::vector<std::reference_wrapper<const std::string>>& ids)
		{
			auto comparator = [](std::reference_wrapper<const std::string> str1, std::reference_wrapper<const std::string> str2)
			{
				return impl::utils::CompareStringsIgnoreCase()(str1.get(), str2.get());
			};

			std::transform(ids.cbegin(), ids.cend(), std::back_inserter(_ids),
				[](const std::reference_wrapper<const std::string>& id)
			{
				return id.get();
			});

			std::set<std::reference_wrapper<const std::string>, decltype(comparator)> distinct(
				ids.cbegin(), ids.cend(), comparator);

			std::for_each(distinct.cbegin(), distinct.cend(), [this](const std::reference_wrapper<const std::string>& id)
			{
				by_id(id.get());
			});

			return *this;
		}

		template<typename T>
		std::shared_ptr<T> get_document()
		{
			if (_session.get().no_tracking)
			{
				if (!_current_load_results)
				{
					throw std::runtime_error("Cannot execute 'get_documents' before operation execution.");
				}
				if(_current_load_results)
				{
					
					auto document = _current_load_results.value().results.size() > 0 ?
						_current_load_results.value().results.at(0) : nlohmann::json(nullptr);
					if(document.is_null())
					{
						return  nullptr;
					}
					auto doc_info = DocumentInfo(document);
					return _session.get().track_entity<T>(doc_info);
				}else
				{
					return nullptr;
				}
			}
			return get_document<T>(_ids[0]);//TODO check if _ids[0] is OK
		}
		template<typename T>
		std::shared_ptr<T> get_document(const std::string& id)
		{
			if(id.empty())
			{
				return {};
			}
			if(_session.get().is_deleted(id))
			{
				return {};
			}

			if (auto doc_info = _session.get()._documents_by_id.find(id);
				doc_info != _session.get()._documents_by_id.end())
			{
				return _session.get().track_entity<T>(*doc_info->second);
			}
			
			if (auto doc_info = _session.get()._included_documents_by_id.find(id);
				doc_info != _session.get()._included_documents_by_id.end())
			{
				return _session.get().track_entity<T>(*doc_info->second);
			}

			return {};
		}

		template<typename T>
		DocumentsByIdsMap<T> get_documents()
		{
			DocumentsByIdsMap<T> results{};

			if(_session.get().no_tracking)
			{
				if(!_current_load_results)
				{
					throw std::runtime_error("Cannot execute 'get_documents' before operation execution.");
				}
				std::for_each(_ids.cbegin(), _ids.cend(), [&](const std::string& id)
				{
					if(!id.empty())
					{
						results.insert({id, nullptr});
					}
				});

				for(auto& doc: _current_load_results.value().results)
				{
					if(doc.is_null())
					{
						continue;
					}
					auto doc_info = DocumentInfo(doc);
					results.insert({doc_info.id, _session.get().track_entity<T>(doc_info)});
				}
			}

			for(const auto& id : _ids)
			{
				if(!id.empty())
				{
					results.insert({id, get_document<T>(id)});
				}
			}
			return results;
		}

		void set_result(const commands::GetDocumentsResult& result)
		{
			if(_session.get().no_tracking)
			{
				_current_load_results = result;
			}

			//TODO add later _session.get().register_includes(result.includes);

			for(const auto& document : result.results)
			{
				if(document.empty())//null is empty
				{
					continue;
				}

				auto new_doc_info = std::make_shared<DocumentInfo>(document);
				_session.get()._documents_by_id.insert({new_doc_info->id,new_doc_info});
			}

			//TODO add later _session.get().register_missing_includes(result.results, result.includes, _includes);
		}
	};
}
