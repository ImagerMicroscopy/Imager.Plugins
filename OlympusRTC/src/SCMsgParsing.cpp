#include "SCMsgParsing.h"

#include <algorithm>
#include <stdexcept>

bool IsAckMessage(const pugi::xml_document& doc) {
    return true;
    pugi::xpath_node_set nodes = doc.select_nodes("//AckMsg");

    // Check if any nodes were found
    return !nodes.empty();
}

bool ExperimentExecuted(const pugi::xml_document& doc) {
    return true;
    pugi::xml_node node = doc.child("SCMsg").child("ExperimentReport").child("RepText");
    if (node){
        std::string rep_text = node.child_value();
        if (rep_text == "Experiment executed"){
            return true;
        }

    }
    return false;
}


std::vector<std::string> ParseLaserNamesFromDevices(const pugi::xml_document& doc) {
    std::vector<std::string> laserNames;

    pugi::xpath_node_set laserDevices = doc.select_nodes("//Devices/Device[contains(@Id, 'Laser')]");
    for (const auto& node : laserDevices) {
        pugi::xml_node device = node.node();
        std::string id = device.attribute("Id").value();
        std::string access = device.attribute("Access").value();
        if (access != "na"){
            laserNames.push_back(id);
        }
    }

    return laserNames;
}

std::vector<std::string> ParseMotorNamesFromDevices(const pugi::xml_document& doc) {
    std::vector<std::string> motorNames;

    pugi::xpath_node_set motorDevices = doc.select_nodes("//Devices/Device[contains(@Id, 'GenMot')]");
    for (const auto& node : motorDevices) {
        pugi::xml_node device = node.node();
        std::string id = device.attribute("Id").value();
        std::string access = device.attribute("Access").value();
        if (access != "na"){
            motorNames.push_back(id);
        }
        
    }

    return motorNames;
}

Laser ParseLaserDetails(const pugi::xml_document& doc) {
    
    // Find the Device node that has Alloc children
    pugi::xpath_node_set alloc_devices = doc.select_nodes("//Setting/Device[Alloc]");
    pugi::xml_node setting_node = doc.select_nodes("//Setting").first().node();


    pugi::xpath_node node = alloc_devices.first();
    pugi::xml_node device = node.node();
    pugi::xml_node config = device.child("Config");

    std::string deviceID = setting_node.attribute("Id").value();
    std::vector<int> laserPowers, transValues;
    int numofPositions = std::stoi(config.attribute("NumofPositions").value());
    for (pugi::xml_node alloc : device.children("Alloc")) {
        laserPowers.push_back(std::stoi(alloc.attribute("Pos").value()));
        transValues.push_back(std::stoi(alloc.attribute("Trans").value()));
    }
    bool hasDiscreteSettings = (numofPositions == laserPowers.size());

    // Find the Wavelength element and its value
    pugi::xpath_node wavelength_node = doc.select_node("//Device[Config[@Wavelength]]");
    std::string wavelength = wavelength_node.node().child("Config").attribute("Wavelength").value();


    Laser laser;
    laser.name = deviceID;
    laser.hasDiscreteSettings = hasDiscreteSettings;
    laser.maxSetting = numofPositions - 1;
    laser.allowablePowers = laserPowers;
    laser.transValues = transValues;
    laser.wavelength = wavelength;

    return laser;
}

std::vector<Motor> ParseMotorDetails(const pugi::xml_document& doc) {
    // Select all Axis elements
    pugi::xpath_node_set axis_nodes = doc.select_nodes("//Setting/Device/Axis");

    std::vector<Motor> motors;
    Motor motor;
    for (const auto& node : axis_nodes) {
        pugi::xml_node axis = node.node();
        pugi::xml_node device = axis.parent();

        motor.motorName = device.attribute("Id").value();
        motor.axisID = std::stoi(axis.attribute("Type").value());
        motor.lowVal = std::stoi(axis.attribute("LowRange").value());
        motor.highVal = std::stoi(axis.attribute("UpRange").value());
        motors.push_back(motor);
    }

    return motors;
}
