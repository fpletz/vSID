#include "pch.h"

#include "configparser.h"
//#include <string>
//#include <vector>

//#include <filesystem>

//#include <fstream>

//#include "EuroScopePlugIn.h"

//#include "utils.h"

//#include "vSID.h"
//#include "messageHandler.h"

//#include "nlohmann/json.hpp"

//namespace fs = std::filesystem;
//using json = nlohmann::ordered_json;

#include "messageHandler.h"

vsid::ConfigParser::ConfigParser()
{
};

vsid::ConfigParser::~ConfigParser() = default;

void vsid::ConfigParser::loadMainConfig()
{
    char path[MAX_PATH + 1] = { 0 };
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    std::filesystem::path basePath = path;

    std::ifstream configFile(basePath.append("vSidConfig.json").string());

    try
    {
        this->vSidConfig = json::parse(configFile);
    }
    catch(const json::parse_error &e)
    {
        //vsid::messagehandler::LogMessage("ERROR", "Failed to parse main config: " + std::string(e.what()));
        messageHandler->writeMessage("ERROR", "Failed to parse main config: " + std::string(e.what()));
    }
    catch (const json::type_error& e)
    {
        //vsid::messagehandler::LogMessage("ERROR", "Failed to parse main config: " + std::string(e.what()));
        messageHandler->writeMessage("ERROR", "Failed to parse main config: " + std::string(e.what()));
    }
    try
    {
        for (auto &elem : this->vSidConfig.at("colors").items())
        {
            // default values are cyan to signal if the import went wront
            COLORREF rgbColor = RGB(
                this->vSidConfig.at("colors").at(elem.key()).value("r", 30),
                this->vSidConfig.at("colors").at(elem.key()).value("g", 200),
                this->vSidConfig.at("colors").at(elem.key()).value("b", 230),
                );
            this->colors[elem.key()] = rgbColor;
        }
    }
    catch (std::error_code& e)
    {
        messageHandler->writeMessage("ERROR", "Failed to import colors: " + e.message());
    }
}

void vsid::ConfigParser::loadAirportConfig(std::vector<vsid::airport> &activeAirports)
{
    // get the current path where plugins .dll is stored
    char path[MAX_PATH + 1] = { 0 };
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    std::filesystem::path basePath = path;
    //basePath.concat("\\config");
    if (this->vSidConfig.contains("airportConfigs"))
    {
        basePath.append(this->vSidConfig.value("airportConfigs", ""));
        messageHandler->writeMessage("DEBUG", "airport config: " + basePath.string());
    }
    else
    {
        messageHandler->writeMessage("ERROR", "No config path for airports in main config");
        return;
    }

    std::vector<std::filesystem::path> files;
    for (vsid::airport& airport : activeAirports)
    {
        for (const std::filesystem::path& entry : std::filesystem::directory_iterator(basePath))
        {
            if (entry.extension() == ".json")
            {
                std::ifstream configFile(entry.string());

                try
                {
                    //configTest = json::parse(testFile);
                    this->parsedConfig = json::parse(configFile);

                    if (!this->parsedConfig.contains(airport.icao))
                    {
                        continue;
                    }
                    else
                    {
                        airport.elevation = this->parsedConfig.at(airport.icao).value("elevation", 0);
                        airport.transAlt = this->parsedConfig.at(airport.icao).value("transAlt", 0);
                        airport.maxInitialClimb = this->parsedConfig.at(airport.icao).value("maxInitialClimb", 0);
                        for (vsid::sids::sid& sid : airport.sids)
                        {
                            if (this->parsedConfig.at(airport.icao).at("sids").contains(sid.waypoint))
                            {
                                json configWaypoint =   this->parsedConfig.
                                                        at(airport.icao).
                                                        at("sids").
                                                        at(sid.waypoint);
                                std::string designator{ sid.designator };
                                if (this->parsedConfig.at(airport.icao).at("sids").at(sid.waypoint).contains(designator))
                                {
                                    sid.rwy = configWaypoint.at(designator).value("rwy", "");
                                    sid.initialClimb = configWaypoint.at(designator).value("initial", 0);
                                    sid.climbvia = configWaypoint.at(designator).value("climbvia", 0);
                                    sid.prio = configWaypoint.at(designator).value("prio", 0);
                                    sid.pilotfiled = configWaypoint.at(designator).value("pilotfiled", 0);
                                    sid.wtc = configWaypoint.at(designator).value("wtc", "");
                                    sid.engineType = configWaypoint.at(designator).value("engineType", "");
                                    sid.engineCount = configWaypoint.at(designator).value("engineCount", 0);
                                    sid.mtow = configWaypoint.at(designator).value("mtow", 0);                                                                      
                                }
                            }
                        }
                    }
                }
                catch (const json::parse_error& e)
                {
                    //vsid::messagehandler::LogMessage("ERROR:", "Failed to load airport config : " + std::string(e.what
                    messageHandler->writeMessage("ERROR:", "Failed to load airport config : " + std::string(e.what()));
                }
                catch (const json::type_error& e)
                {
                    //vsid::messagehandler::LogMessage("ERROR:", "Failed to load airport config : " + std::string(e.what()));
                    messageHandler->writeMessage("ERROR:", "Failed to load airport config : " + std::string(e.what()));
                }

                /* DOCUMENTATION on how to get all values below a key
                json waypoint = this->configFile.at("EDDF").at("sids").at("MARUN");
                for (auto it : waypoint.items())
                {
                    vsid::messagehandler::LogMessage("JSON it:", it.value().dump());
                }*/
            }
        }
    }    
}

COLORREF vsid::ConfigParser::getColor(std::string color)
{
    if (auto &elem = this->colors.find(color); elem != this->colors.end())
    {
        return this->colors[color];
    }
    else
    {
        messageHandler->writeMessage("ERROR", "Failed to retrieve color: \"" + color + "\"");
        // return if color could not be found is purple to signal error
        COLORREF rgbColor = RGB(190, 30, 190);
        return rgbColor;
    }
}

