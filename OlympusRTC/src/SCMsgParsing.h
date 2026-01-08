#ifndef SCMSGPARSING_H
#define SCMSGPARSING_H

#include <string>
#include <vector>

#include "pugixml.hpp"

struct Laser {
    std::string name;
    std::string wavelength;
    bool hasDiscreteSettings;
    std::vector<int> allowablePowers;
    std::vector<int> transValues;
    int maxSetting;
};

struct Motor {
    std::string motorName;
    int axisID;
    int lowVal;
    int highVal;
};

bool IsAckMessage(const pugi::xml_document& doc);
bool ExperimentExecuted(const pugi::xml_document& doc);

std::vector<std::string> ParseLaserNamesFromDevices(const pugi::xml_document& doc);
std::vector<std::string> ParseMotorNamesFromDevices(const pugi::xml_document& doc);

Laser ParseLaserDetails(const pugi::xml_document& doc);
std::vector<Motor> ParseMotorDetails(const pugi::xml_document& doc);


#endif
