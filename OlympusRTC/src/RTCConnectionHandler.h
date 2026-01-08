#ifndef RTCCONNECTIONHANDLER_H
#define RTCCONNECTIONHANDLER_H

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
};

#endif
