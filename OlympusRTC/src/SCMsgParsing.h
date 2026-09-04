#ifndef SCMSGPARSING_H
#define SCMSGPARSING_H

#include <string>
#include <vector>

#include "pugixml.hpp"

struct Laser {
    std::string name;              // device ID, e.g. "CAN.0-Laser.0"
    std::string wavelength;
    bool hasDiscreteSettings = false;
    int numofPositions = 0;        // valid <Position> values are [0, numofPositions-1]
    double minPowerPercent = 0.0;
    double maxPowerPercent = 100.0;
};

struct Motor {
    std::string motorName;         // device ID, e.g. "CAN.0-GenMot.0"
    int axisID = 0;
    int lowStep = 0;
    int highStep = 0;
    std::string displayName;
};

bool IsAckMessage(const pugi::xml_document& doc);
bool ExperimentExecuted(const pugi::xml_document& doc);

std::vector<std::string> ParseLaserNamesFromDevices(const pugi::xml_document& doc);
std::vector<std::string> ParseMotorNamesFromDevices(const pugi::xml_document& doc);

Laser ParseLaserDetails(const pugi::xml_document& doc);
std::vector<Motor> ParseMotorDetails(const pugi::xml_document& doc);


#endif
