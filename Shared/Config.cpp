// Generic INI configuration file parser implementation
// Used by client for config.ini settings

#include "Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// Helper: Trim whitespace from both ends of a string
static std::string trim(const std::string& str)
{
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool Config::load(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    m_data.clear();
    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Section header: [SectionName]
        if (line[0] == '[') {
            auto endBracket = line.find(']');
            if (endBracket == std::string::npos) {
                continue;
            }
            currentSection = line.substr(1, endBracket - 1);
            continue;
        }

        // Key=Value pair
        auto equalPos = line.find('=');
        if (equalPos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, equalPos));
        std::string value = trim(line.substr(equalPos + 1));

        // Remove quotes from value if present
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        m_data[currentSection][key] = value;
    }

    return true;
}

bool Config::save(const std::string& filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& [section, keys] : m_data) {
        file << "[" << section << "]\n";
        for (const auto& [key, value] : keys) {
            file << key << "=" << value << "\n";
        }
        file << "\n";
    }

    return true;
}

std::string Config::getString(const std::string& section, const std::string& key, const std::string& defaultVal) const
{
    auto secIt = m_data.find(section);
    if (secIt == m_data.end()) return defaultVal;
    auto keyIt = secIt->second.find(key);
    if (keyIt == secIt->second.end()) return defaultVal;
    return keyIt->second;
}

int Config::getInt(const std::string& section, const std::string& key, int defaultVal) const
{
    std::string val = getString(section, key, "");
    if (val.empty()) return defaultVal;
    try {
        return std::stoi(val);
    } catch (...) {
        return defaultVal;
    }
}

float Config::getFloat(const std::string& section, const std::string& key, float defaultVal) const
{
    std::string val = getString(section, key, "");
    if (val.empty()) return defaultVal;
    try {
        return std::stof(val);
    } catch (...) {
        return defaultVal;
    }
}

bool Config::getBool(const std::string& section, const std::string& key, bool defaultVal) const
{
    std::string val = getString(section, key, "");
    if (val.empty()) return defaultVal;

    // Handle various boolean representations
    std::string lower = val;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        return true;
    }
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        return false;
    }
    return defaultVal;
}

void Config::setString(const std::string& section, const std::string& key, const std::string& value)
{
    m_data[section][key] = value;
}

void Config::setInt(const std::string& section, const std::string& key, int value)
{
    m_data[section][key] = std::to_string(value);
}

void Config::setFloat(const std::string& section, const std::string& key, float value)
{
    std::ostringstream oss;
    oss << value;
    m_data[section][key] = oss.str();
}

void Config::setBool(const std::string& section, const std::string& key, bool value)
{
    m_data[section][key] = value ? "true" : "false";
}
