#include <iostream>
#include "srcs/http/HttpRequest.hpp"
#include "srcs/http/HttpResponse.hpp"

int main()
{
	std::cout << "=== Testing HttpRequest ===" << std::endl << std::endl;

	// Test 1: Simple GET request
	HttpRequest request;
	std::string rawRequest =
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"User-Agent: TestClient/1.0\r\n"
		"\r\n";

	request.appendData(rawRequest);

	if (request.isComplete())
	{
		std::cout << "✓ Request parsed successfully!" << std::endl;
		std::cout << "  Method: " << request.getMethod() << std::endl;
		std::cout << "  URI: " << request.getUri() << std::endl;
		std::cout << "  Version: " << request.getVersion() << std::endl;

		const std::map<std::string, std::string> &headers = request.getHeaders();
		std::cout << "  Headers:" << std::endl;
		for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		{
			std::cout << "    " << it->first << ": " << it->second << std::endl;
		}
	}
	else
	{
		std::cout << "✗ Request parsing failed!" << std::endl;
	}

	// Test 2: POST request with body
	std::cout << std::endl << "--- Testing POST with body ---" << std::endl;
	HttpRequest postRequest;
	std::string postData =
		"POST /api/data HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Length: 27\r\n"
		"\r\n"
		"{\"key\":\"value\",\"test\":1}";

	postRequest.appendData(postData);

	if (postRequest.isComplete())
	{
		std::cout << "✓ POST request parsed!" << std::endl;
		std::cout << "  Method: " << postRequest.getMethod() << std::endl;
		std::cout << "  URI: " << postRequest.getUri() << std::endl;
	}
	else
	{
		std::cout << "✗ POST request parsing failed!" << std::endl;
	}

	// Test 3: HttpResponse
	std::cout << std::endl << "=== Testing HttpResponse ===" << std::endl << std::endl;

	HttpResponse response;
	response.setStatus(200);
	response.setHeader("Content-Type", "text/html");
	response.setBody("<html><body><h1>Hello World!</h1></body></html>");

	std::string rawResponse = response.build();
	std::cout << "✓ Response built successfully!" << std::endl;
	std::cout << "--- Raw Response ---" << std::endl;
	std::cout << rawResponse << std::endl;

	// Test 4: Error response
	std::cout << std::endl << "--- Testing 404 Error Response ---" << std::endl;
	HttpResponse errorResponse;
	errorResponse.setStatus(404);
	errorResponse.setHeader("Content-Type", "text/html");
	errorResponse.setBody("<html><body><h1>404 Not Found</h1></body></html>");

	std::string errorRaw = errorResponse.build();
	std::cout << errorRaw << std::endl;

	return 0;
}
