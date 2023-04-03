#pragma once

#include "geography.h"

class Station {
private:
    Location m_location;
    std::string m_name;
    std::string m_OACI;
    std::string m_status;
    std::string m_nightVFR;
    std::string m_fuel;

public:
    Station(const Location &location, const std::string &name, const std::string &OACI, const std::string &status, const std::string &nightVFR, const std::string &fuel)
      : m_location(location)
    {
        m_name = name;
        m_OACI = OACI;
        m_status = status;
        m_nightVFR = nightVFR;
        m_fuel = fuel;
    }

    const Location &getLocation() const { return m_location; }
    const std::string &getName() const { return m_name; }
    const std::string &getOACI() const { return m_OACI; }
    const std::string &getStatus() const { return m_status; }
    const std::string &getNightVFR() const { return m_nightVFR; }
    const std::string &getFuel() const { return m_fuel; }

    bool operator==(const Station &other) const { return m_location == other.m_location; }
    bool operator!=(const Station &other) const { return m_location != other.m_location; }
};