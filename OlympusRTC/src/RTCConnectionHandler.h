#ifndef RTCCONNECTIONHANDLER_H
#define RTCCONNECTIONHANDLER_H

#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "pugixml.hpp"
int countOccurrences(const std::string& text, const std::string& toFind);

class RTCConnectionHandler {
public:
    RTCConnectionHandler();
    ~RTCConnectionHandler();

    void acceptConnection();

    void sendXMLMessage(const pugi::xml_document& xmlMessage);
    pugi::xml_document receiveNextSCMessage();

private:

    SOCKET _socket;

    // Bytes read past the end of the most recently extracted message. These
    // belong to the next message and must survive across calls.
    std::string _recvBuffer;
};

#endif
