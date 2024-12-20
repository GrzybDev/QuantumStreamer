#pragma once
#include "VideoList.hpp"

class StreamingServerConnection : public std::enable_shared_from_this<StreamingServerConnection>
{
public:
	explicit StreamingServerConnection(boost::asio::ip::tcp::socket socket,
	                                   const std::shared_ptr<HttpClient>& httpClient);
	void Start();

private:
	std::shared_ptr<HttpClient> httpClient_;

	// The socket for the currently connected client.
	boost::asio::ip::tcp::socket socket_;
	// The buffer for performing reads.
	boost::beast::flat_buffer buffer_{8192};

	// The request message.
	boost::beast::http::request<boost::beast::http::dynamic_body> request_;

	// The response message.
	boost::beast::http::response<boost::beast::http::dynamic_body> response_;

	VideoList videoList = VideoList::GetInstance();

	void ReadRequest();
	void ProcessRequest();
	void PrepareManifest(std::string basePath, std::string manifestUrl);
	void PrepareSegment(std::string basePath, std::string manifestUrl, std::string action, std::string episode);
	void WriteResponse();
};
