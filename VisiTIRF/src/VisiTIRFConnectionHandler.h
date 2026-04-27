#ifndef VISITIRFCONNECTIONHANDLER_H
#define VISITIRFCONNECTIONHANDLER_H

#include <vector>
#include <string>
#include <Windows.h>

#include "ImagerPluginCore/DeviceTemplates.h"

class VisiTIRFConnectionHandler {
public:
    VisiTIRFConnectionHandler();
    ~VisiTIRFConnectionHandler();

    VisiTIRFConnectionHandler(const VisiTIRFConnectionHandler&) = delete;
    VisiTIRFConnectionHandler& operator=(const VisiTIRFConnectionHandler&) = delete;

    std::vector<std::shared_ptr<ContinuouslyMovableComponent>> getContinuouslyMovableComponents();

    void startConnection();
    void closeConnection();

private:
    void setSetting(std::string setting,double value);
    void sendSettings();

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