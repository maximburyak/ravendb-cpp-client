// Tryouts.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#include "GetDocumentsCommand.h"
#include "PutDocumentCommand.h"
#include "RequestExecutor.h"
#include "GetDatabaseRecordCommand.h"
#include "CreateDatabaseCommand.h"
#include "DeleteDocumentCommand.h"
#include "ravendb.h"
#include "GetDatabaseNamesCommand.h"


//using fiddler + verbose
CURL* debug_curl_init(void*)
{
	auto curl = curl_easy_init();
	if (curl == nullptr)
	{
		std::runtime_error("debug_curl_init failed");
	}
	//for fiddler
	curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:8888");

	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	return curl;
}

int main()
{
	using namespace ravenDB;

	RavenGlobalInit::make_global_initializations();

	std::unique_ptr<RequestExecutor> rq;
	try
	{
		rq = RequestExecutor::create({ "http://127.0.0.1:8081" }, {} , {}, { debug_curl_init, nullptr });
	}
	catch (RavenError &re)
	{
		std::cout << re.what() << std::endl;
		return 1;
	}

	GetDatabaseNamesCommand cmd(0,10);

	std::vector<std::string> result;

	try
	{
		result = rq->execute(cmd);
	}
	catch (RavenError &re)
	{
		std::cout << re.what() << std::endl;
		return 2;
	}

	std::cout << "DB names are :\n";
	for (auto& element : result)
	{
		std::cout << element << '\n';
	}














}

