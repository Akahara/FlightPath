#pragma once

#include "geography.h"

class Station {
public:
    enum class NightVFR { NO, YES, LIMITED, PCL, UNKNOWN };
    enum class Fuel { NO, YES, AUTOMAT, UNKNOWN };

private:
    Location m_location;
    std::string m_name;
    std::string m_OACI;
    std::string m_status;
    NightVFR m_nightVFR;
    Fuel m_fuel;

public:
    Station(const Location &location, const std::string &name, const std::string &OACI, const std::string &status, NightVFR nightVFR, Fuel fuel)
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

    const NightVFR& getNightVFR() const { return m_nightVFR; }
    const Fuel& getFuel() const { return m_fuel; }
    const std::string& getStatus() const { return m_status; }

    bool operator==(const Station& other) const { return m_location == other.m_location; }
};