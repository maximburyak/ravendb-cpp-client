#include "stdafx.h"
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#include "utils.h"
#include "CertificateDetails.h"

namespace
{
	void ltrim(std::string &s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
		}));
	}

	void rtrim(std::string &s) 
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}
}

namespace ravendb::client::impl::utils
{
	size_t utils::push_headers(char *buffer, size_t size, size_t nitems, void *userdata)
	{
		auto real_size = size * nitems;
		auto headers = static_cast<std::map<std::string, std::string>*>(userdata);
		auto result = static_cast<char*>(memchr(buffer, ':', real_size));
		if (result == nullptr) // doesn't have ':', probably not a header line, not interesting
			return real_size;

		auto header_name_size = result - buffer;

		auto header_name = std::string(buffer, header_name_size);

		auto header_val = std::string(result + 1, real_size - header_name_size - 1);
		// remove starting space and \r\n at end
		ltrim(header_val);
		rtrim(header_val);

		headers->emplace(header_name, header_val);

		return real_size;
	}

	size_t utils::push_to_buffer(char* contents, size_t size, size_t nmemb, void* output_buffer_void)
	{
		auto real_size = size * nmemb;
		auto& output_buffer = *static_cast<std::string*>(output_buffer_void);
		output_buffer.append((char*)contents, real_size);
		return real_size;
	}

	std::string utils::url_escape(CURL* curl, const std::string& val)
	{
		auto result = curl_easy_escape(curl, val.c_str(), (int)val.length());
		if (result == nullptr)
		{
			throw std::invalid_argument("url escape failure on: " + val);
		}
		auto str = std::string(result);
		curl_free(result);
		return str;
	}

	size_t utils::read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
	{
		auto str = static_cast<const std::string*>(stream);
		size_t length = str->length();

		std::memcpy(ptr, str->data(), length);
		return length;
	}

	CURLcode utils::sslctx_function(CURL*, void *sslctx_void, void *cert_details_void)
	{
		static const auto my_BIO_free = [&](BIO* bio) {if (bio != nullptr) BIO_vfree(bio); };
		static const auto my_X509_free = [&](X509* cert) {if (cert != nullptr) X509_free(cert); };
		static const auto my_RSA_free = [&](RSA *rsa) {if (rsa != nullptr) RSA_free(rsa); };

		SSL_CTX* sslctx = static_cast<SSL_CTX*>(sslctx_void);
		const CertificateDetails& cert_details = *static_cast<const CertificateDetails*>(cert_details_void);

		//create a bio for the certificate
		auto cert_bio = std::unique_ptr < BIO, decltype(my_BIO_free) >
			(BIO_new_mem_buf
			(cert_details.certificate.c_str(), (int)cert_details.certificate.length()), my_BIO_free);
		if (!cert_bio)
		{
			throw std::runtime_error("BIO_new_mem_buf failed\n");
		}

		auto cert = std::unique_ptr < X509, decltype(my_X509_free) >
			(PEM_read_bio_X509(cert_bio.get(), nullptr, nullptr, nullptr), my_X509_free);
		if (!cert)
		{
			throw std::runtime_error("PEM_read_bio_X509 failed\n");
		}

		//tell SSL to use the X509 certificate
		if (auto ret = SSL_CTX_use_certificate(sslctx, cert.get()); ret != 1)
		{
			throw std::runtime_error("Use certificate failed\n");
		}

		//create a bio for the RSA key
		auto key_bio = std::unique_ptr < BIO, decltype(my_BIO_free) >
			(BIO_new_mem_buf(cert_details.key.c_str(), (int)cert_details.key.length()), my_BIO_free);
		if (!key_bio)
		{
			throw std::runtime_error("BIO_new_mem_buf failed\n");
		}

		//read the key bio into an RSA object
		auto rsa = std::unique_ptr < RSA, decltype(my_RSA_free) >
			(PEM_read_bio_RSAPrivateKey(key_bio.get(), nullptr, nullptr,
				const_cast<char*>(cert_details.key_password.has_value() ? cert_details.key_password.value().c_str() : nullptr)),
				my_RSA_free);
		if (!rsa)
		{
			throw std::runtime_error("Failed to create key bio\n");
		}

		//tell SSL to use the RSA key
		if (auto ret = SSL_CTX_use_RSAPrivateKey(sslctx, rsa.get()); ret != 1)
		{
			throw std::runtime_error("Use Key failed\n");
		}

		// all set to go
		return CURLE_OK;
	}
}