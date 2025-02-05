#include "GameConfig.h"
#include <filesystem>
#include <fstream>
#include <vector>

using namespace std::chrono_literals;

GameConfig::GameConfig() : ini::IniFile() 
{ 
	if (!LoadFromFile())
	{ 
		std::abort(); 
	}
}

GameConfig::~GameConfig() 
{
	(*this).save("../saved.ini");
}

bool GameConfig::LoadIniFile(const std::string& filePath) 
{
    std::ifstream file(filePath, std::ios::binary);  // Open in binary mode

    if (!file.is_open()) {
        return false;  // File could not be opened
    }

    // Read file contents into a vector
    std::vector<char> fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();  // Close file after reading

    // Check and remove UTF-8 BOM if present
    bool hasBOM = (fileContent.size() >= 3 &&
        static_cast<unsigned char>(fileContent[0]) == 0xEF &&
        static_cast<unsigned char>(fileContent[1]) == 0xBB &&
        static_cast<unsigned char>(fileContent[2]) == 0xBF);

    if (hasBOM) {
        fileContent.erase(fileContent.begin(), fileContent.begin() + 3);  // Remove BOM
        // Overwrite the existing file with BOM removed
        std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open()) {
            return false;  // Failed to overwrite the file
        }
        outFile.write(fileContent.data(), fileContent.size());
        outFile.close();
    }

    // Load the INI file
    (*this).load(filePath);
    return true;
}


bool GameConfig::LoadFromFile()
{
    const std::string defaults = "../defaults.ini";
    const std::string saved = "../saved.ini";

    if (std::filesystem::exists(defaults) && std::filesystem::exists(saved)) {
        auto defaultsTime = std::filesystem::last_write_time(defaults);
        auto savedTime = std::filesystem::last_write_time(saved);

        std::cout << "Defaults Last Modified: " << defaultsTime.time_since_epoch().count() << std::endl;
        std::cout << "Saved Last Modified: " << savedTime.time_since_epoch().count() << std::endl;

        if (std::filesystem::exists(defaults)) 
        {
            return LoadIniFile("../defaults.ini");
        }

        //// Load the newest file
        //if (defaultsTime > savedTime)
        //    return LoadIniFile("../defaults.ini");
        //else
        //    return LoadIniFile("../saved.ini");
    }
    /*else if (std::filesystem::exists(defaults)) {
        return LoadIniFile("../defaults.ini");
            
    }*/

    return false;  // No valid INI file found
}
