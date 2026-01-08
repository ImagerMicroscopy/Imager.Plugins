#ifndef VISITIRFCONNECTIONHANDLER_H
#define VISITIRFCONNECTIONHANDLER_H

#include <vector>
#include <string>
#include <Windows.h>

class VisiTIRFConnectionHandler {
public:
    VisiTIRFConnectionHandler();
    ~VisiTIRFConnectionHandler();

    void startConnection();
    void closeConnection();
    void setSetting(std::string setting,double value);
    void sendSettings();

private:
    void sendString(const std::string& str);
    std::vector<std::string> listCOMPorts();
    bool probePort(const std::string& port);

    std::string connectedPort;
    HANDLE hSerial;

    double xAmplitude;
    double yAmplitude;
    double xOffset;
    double yOffset;
    double phi;
    double tExp;
};
#endif