#include "lagrange/core/NetworkClient.hpp"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

namespace lagrange {
namespace core {

NetworkClient::NetworkClient(const Config& cfg) : config(cfg) {}

bool NetworkClient::sendTelemetry(const std::string& serializedPayload) {
    try {
        // 1. Setup the Boost networking context
        boost::asio::io_context io_context;
        
        // 2. For Resolve the host (eg convert "127.0.0.1" and "8080" into an endpoint)
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(config.getBackendHost(), config.getBackendPort());

        // 3. Create a socket and connect it to the Spring Boot server
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        // 4. Manually construct the HTTP POST request headers
        std::string request = 
            "POST " + config.getBackendPath() + " HTTP/1.1\r\n"
            "Host: " + config.getBackendHost() + ":" + config.getBackendPort() + "\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Length: " + std::to_string(serializedPayload.length()) + "\r\n"
            "Connection: close\r\n\r\n" +  // The double \r\n\r\n separates headers from the payload
            serializedPayload;

        // 5. Send the entire request over the socket
        boost::asio::write(socket, boost::asio::buffer(request));

        // 6. Close the socket safely
        boost::system::error_code error;
        socket.close(error);
        
        return true;
    } catch (std::exception& e) {
        std::cerr << "[NetworkClient] Failed to send telemetry: " << e.what() << std::endl;
        return false;
    }
}
} 
} 