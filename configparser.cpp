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

void vsid::ConfigParser::loadAirportConfig(std::map<std::string, vsid::airport> &activeAirports)
{
    // get the current path where plugins .dll is stored
    char path[MAX_PATH + 1] = { 0 };
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    std::filesystem::path basePath = path;
    //basePath.concat("\\config");
    if (this->vSidConfig.contains("airportConfigs"))
    {
        basePath.append(this->vSidConfig.value("airportConfigs", "")).make_preferred();
        messageHandler->writeMessage("DEBUG", "airport config: " + basePath.string());
    }
    else
    {
        messageHandler->writeMessage("ERROR", "No config path for airports in main config");
        return;
    }

    if (!std::filesystem::exists(basePath))
    {
        messageHandler->writeMessage("ERROR", "No airport config folder found at: " + basePath.string());
        return;
    }

    std::vector<std::filesystem::path> files;
    std::set<std::string> aptConfig;
    for (std::pair<const std::string, vsid::airport> &apt : activeAirports)
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

                    if (!this->parsedConfig.contains(apt.first))
                    {
                        continue;
                    }
                    else
                    {
                        aptConfig.insert(apt.first);

                        apt.second.elevation = this->parsedConfig.at(apt.first).value("elevation", 0);
                        apt.second.transAlt = this->parsedConfig.at(apt.first).value("transAlt", 0);
                        apt.second.maxInitialClimb = this->parsedConfig.at(apt.first).value("maxInitialClimb", 0);

                        for (auto &sid : this->parsedConfig.at(apt.first).at("sids").items())
                        {
                            std::string wpt = sid.key(); // sid waypoint

                            for (auto& sidWpt : this->parsedConfig.at(apt.first).at("sids").at(sid.key()).items())
                            {
                                std::string desig = sidWpt.key(); // sid designator, [0] operand needed as string is mandatory here

                                std::string rwys = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("rwy", "");
                                int initial = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("initial", 0);
                                int via = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("climbvia", 0);
                                int prio = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("prio", 99);
                                int pilot = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("pilotfiled", 0);
                                std::string wtc = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("wtc", "");
                                std::string engineType = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("engineType", "");
                                int engineCount = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("engineCount", 0);
                                int mtow = this->parsedConfig.at(apt.first).at("sids").at(sid.key()).at(sidWpt.key()).value("mtow", 0);
                                // area when implemented
                                // equip when implemented
                                // lvo when implemented
                                // timeFrom when implemented
                                // timeTo when implemented
                                
                                vsid::sids::sid newSid = {  wpt,
                                                            ' ',
                                                            desig[0],
                                                            rwys,
                                                            initial,
                                                            via,
                                                            prio,
                                                            pilot,
                                                            wtc,
                                                            engineType,
                                                            engineCount,
                                                            mtow,
                                                         };
                                apt.second.sids.push_back(newSid);
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
    for (std::pair<const std::string, vsid::airport>& apt : activeAirports)
    {
        if (aptConfig.count(apt.first)) continue;
        messageHandler->writeMessage("INFO", "No config found for: " + apt.first);
    }
}

void vsid::ConfigParser::loadGrpConfig()
{
    // get the current path where plugins .dll is stored
    char path[MAX_PATH + 1] = { 0 };
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    std::filesystem::path basePath = path;
    //basePath.concat("\\config");
    if (this->vSidConfig.contains("grp"))
    {
        basePath.append(this->vSidConfig.value("grp", "")).make_preferred();
        messageHandler->writeMessage("DEBUG", "grp config: " + basePath.string());
    }
    else
    {
        messageHandler->writeMessage("ERROR", "No config path for GRP in main config");
        return;
    }

    if (!std::filesystem::exists(basePath))
    {
        messageHandler->writeMessage("ERROR", "No grp config found in: " + basePath.string());
        return;
    }
    for (const std::filesystem::path& entry : std::filesystem::directory_iterator(basePath))
    {
        if (entry.extension() == ".json")
        {
            if (entry.filename().string() != "ICAO_Aircraft.json") continue;

            std::ifstream configFile(entry.string());

            try
            {
                this->grpConfig = json::parse(configFile);

                //int i = 0;
                //for (auto it = this->grpConfig.begin(); it != this->grpConfig.end(); ++it)
                //{
                //    if (i > 5) break;
                //    if (it->contains("MTOW"))
                //    {
                //        //messageHandler->writeMessage("DEBUG", std::to_string(it->value("MTOW", 0)));
                //    }
                //    i++;
                //}
            }
            catch (const json::parse_error& e)
            {
                //vsid::messagehandler::LogMessage("ERROR:", "Failed to load airport config : " + std::string(e.what
                messageHandler->writeMessage("ERROR:", "Failed to load grp config : " + std::string(e.what()));
            }
            catch (const json::type_error& e)
            {
                //vsid::messagehandler::LogMessage("ERROR:", "Failed to load airport config : " + std::string(e.what()));
                messageHandler->writeMessage("ERROR:", "Failed to load grp config : " + std::string(e.what()));
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

