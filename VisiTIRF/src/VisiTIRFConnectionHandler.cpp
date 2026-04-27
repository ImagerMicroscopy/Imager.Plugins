#include "VisiTIRFConnectionHandler.h"

VisiTIRFConnectionHandler::VisiTIRFConnectionHandler() : hSerial(INVALID_HANDLE_VALUE)
{
    // Constructor started: initialize internals and enumerate COM ports
    auto ports = listCOMPorts();
    // Port enumeration completed
    if (ports.empty())
    {
        // No COM ports found: nothing to connect to
        return;
    }

    for (const auto &port : ports)
    {
        // Probing the current port to see if it matches the device
        if (probePort(port))
        {
            connectedPort = port;
            // Connected: `connectedPort` set to this `port`
            return;
        }
    }
}

VisiTIRFConnectionHandler::~VisiTIRFConnectionHandler()
{
    closeConnection();
}

void VisiTIRFConnectionHandler::closeConnection()
{
    if (hSerial != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        // Connection handle closed and reset
    }
}

void VisiTIRFConnectionHandler::sendString(const std::string &message)
{
    if (hSerial == INVALID_HANDLE_VALUE)
    {
        // No open connection available to send data
        return;
    }

    DWORD bytesWritten;
    WriteFile(hSerial, message.c_str(), static_cast<DWORD>(message.size()), &bytesWritten, NULL);
}

std::vector<std::shared_ptr<ContinuouslyMovableComponent>> VisiTIRFConnectionHandler::getContinuouslyMovableComponents() {
    std::vector<std::shared_ptr<ContinuouslyMovableComponent>> components;

    for (const auto& setting : {"X amplitude", "Y amplitude", "X offset", "Y offset", "phi"}) {
        components.push_back(std::make_shared<ContinuouslyMovableComponentFunctor>(
            setting,
            -5.0,   // min value
             5.0, // max value
             0.001,   // increment
            [this, setting](double value) {
                setSetting(setting, value);
            }
        ));
    }

    components.push_back(std::make_shared<ContinuouslyMovableComponentFunctor>(
        "exposure",
        0.0,   // min value
        10, // max value
        0.001,   // increment
        [this](double value) {
            setSetting("exposure", value);
        }
    ));

    return components;
}

void VisiTIRFConnectionHandler::startConnection() {
    if (connectedPort.empty())
        return;

    std::string fullName = "\\\\.\\" + connectedPort;
    hSerial = CreateFileA(
        fullName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        // Failed to open the selected port `connectedPort`
        return;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(hSerial, &dcb);
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(hSerial, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 500;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);
}

std::vector<std::string> VisiTIRFConnectionHandler::listCOMPorts()
{
    std::vector<std::string> ports;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return ports;

    char valueName[256];
    BYTE data[256];
    DWORD valueNameSize, dataSize, type;
    DWORD index = 0;

    while (true)
    {
        valueNameSize = sizeof(valueName);
        dataSize = sizeof(data);
        LONG result = RegEnumValueA(hKey, index, valueName, &valueNameSize, NULL, &type, data, &dataSize);
        if (result == ERROR_NO_MORE_ITEMS)
            break;
        if (result == ERROR_SUCCESS && type == REG_SZ)
            ports.emplace_back(reinterpret_cast<char *>(data));
        index++;
    }

    RegCloseKey(hKey);
    return ports;
}

bool VisiTIRFConnectionHandler::probePort(const std::string &port)
{
    // Attempting to probe port: `port`
    std::string fullName = "\\\\.\\" + port;
    HANDLE testHandle = CreateFileA(
        fullName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (testHandle == INVALID_HANDLE_VALUE)
        return false;

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(testHandle, &dcb))
    {
        CloseHandle(testHandle);
        return false;
    }
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(testHandle, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(testHandle, &timeouts);
    PurgeComm(testHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    const char *cmd = "ver\n";
    DWORD bytesWritten;
    WriteFile(testHandle, cmd, strlen(cmd), &bytesWritten, NULL);

    char buffer[256] = {0};
    DWORD bytesRead;
    bool found = false;
    if (ReadFile(testHandle, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
    {
        buffer[bytesRead] = '\0';
        std::string resp(buffer);
        // Received response from port. Example response content available in `resp`.
        // If the device identifies itself (contains "Orbital Version"), mark as found.
        if (resp.find("Orbital Version") != std::string::npos)
        {
            found = true;
            hSerial = testHandle; // assign the validated handle to class member
        }
    }

    if (!found)
        CloseHandle(testHandle);

    return found;
}

void VisiTIRFConnectionHandler::setSetting(std::string setting, double value)
{
    //TODO: add a configfile in so this values are applied on top of some calibration values
    if (setting == "X amplitude")
    {
        xAmplitude = value;
    }
    else if (setting == "Y amplitude")
    {
        yAmplitude = value;
    }
    else if (setting == "X offset")
    {
        xOffset = value;
    }
    else if (setting == "Y offset")
    {
        yOffset = value;
    }
    else if (setting == "phi")
    {
        phi = value;
    }
    else if (setting == "exposure")
    {
        tExp = value;
    }
}

void VisiTIRFConnectionHandler::sendSettings(){
    char buffer[128];

    // Defaults to ellipse 1
    std::snprintf(buffer, sizeof(buffer), "set %d %g %g %g %g %g %d",
                  1, xOffset, yOffset, xAmplitude, yAmplitude, phi, tExp);

    std::string command(buffer);
    sendString(command);

    // Perhaps this can be changed in the future to be able to select multiple ellipses
    sendString("sel 1");
    sendString("start");
}
