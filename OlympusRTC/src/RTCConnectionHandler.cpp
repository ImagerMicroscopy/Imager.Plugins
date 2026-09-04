#include "RTCConnectionHandler.h"

#include <sstream>
#include <vector>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"
#define DEBUG true


RTCConnectionHandler::RTCConnectionHandler() :
    _socket(0)
{
}

RTCConnectionHandler::~RTCConnectionHandler() {
    if (_socket != 0) {
        closesocket(_socket);
    }
}

void RTCConnectionHandler::acceptConnection() {
    WSADATA wsaData;
    SOCKET server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    // Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create socket: " + std::to_string(WSAGetLastError()));
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set socket options: " + std::to_string(WSAGetLastError()));
    }

    // Define the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4242);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to bind to port: " + std::to_string(WSAGetLastError()));
    }

    // Listen for incoming connections
    if (listen(server_fd, 1) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to listen on socket: " + std::to_string(WSAGetLastError()));
    }

    // Set a timeout for the accept call
    DWORD timeout = 1000; // 1 second
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set socket timeout: " + std::to_string(WSAGetLastError()));
    }

    // Accept the first incoming connection
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == INVALID_SOCKET) {
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            throw std::runtime_error("No connection received within 1 second");
        } else {
            throw std::runtime_error("Failed to accept connection: " + std::to_string(error));
        }
    }

    _socket = client_fd;

    closesocket(server_fd);
}

void RTCConnectionHandler::sendXMLMessage(const pugi::xml_document& xmlMessage) {
    std::stringstream ss;
    xmlMessage.save(ss, "  "); // Save the node (and its children) to a stringstream

    std::string xmlString = ss.str();
    const char* dataToSend = xmlString.c_str();
    int dataLength = static_cast<int>(xmlString.length());

    int offset = 0;
    int nBytesRemaining = dataLength;
    while (nBytesRemaining > 0) {
        int bytesSent = send(_socket, dataToSend + offset, nBytesRemaining, 0);
        if (bytesSent == SOCKET_ERROR) {
            throw std::runtime_error("couldn't write XML message to socket");
        }

        offset += bytesSent;
        nBytesRemaining -= bytesSent;
    }
    if (DEBUG){
        printf("%sSENT DATA (%i bytes)%s\n%s\n",RED,dataLength,RESET,xmlString.c_str());
    }
    
}

pugi::xml_document RTCConnectionHandler::receiveNextSCMessage() {
    // strategy: accumulate bytes from the socket into a persistent buffer
    // until the "</SCMsg>" terminator appears, then split the buffer there.
    // Anything read past the terminator belongs to the next message and is
    // kept in _recvBuffer for the next call, never discarded.

    const std::string terminator("</SCMsg>");
    std::vector<char> buffer(4096);

    size_t terminatorPos = _recvBuffer.find(terminator);
    while (terminatorPos == std::string::npos) {
        int bytesReceived = recv(_socket, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (bytesReceived == SOCKET_ERROR) {
            throw std::runtime_error("recv failed with error: " + std::to_string(WSAGetLastError()));
        }
        if (bytesReceived == 0) {
            throw std::runtime_error("Connection closed by the remote host");
        }

        _recvBuffer.append(buffer.data(), bytesReceived);
        terminatorPos = _recvBuffer.find(terminator);
    }

    size_t messageEnd = terminatorPos + terminator.length();
    std::string message = _recvBuffer.substr(0, messageEnd);
    _recvBuffer.erase(0, messageEnd);

    if (DEBUG){
        printf("%sRECEIVED DATA (%zu bytes)%s\n%s\n",GRN,message.size(),RESET,message.c_str());
    }
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(message.c_str());
    if (!result) {
        throw std::runtime_error("XML parse error on " + message);
    }
    return doc;
}
