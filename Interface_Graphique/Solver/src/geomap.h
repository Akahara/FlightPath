#pragma once

#include <vector>

#include "station.h"
#include "geometry.h"

class GeoMap {
private:
    std::vector<Station> m_stations;

public:
    GeoMap() {}

    const std::vector<Station> &getStations() const { return m_stations; }
    std::vector<Station> &getStations() { return m_stations; }
    void setStations(const std::vector<Station> &stations) { m_stations = stations; }

    std::vector<std::vector<nauticmiles_t>> getDistances() const {
        std::vector<std::vector<nauticmiles_t>> distances(m_stations.size(), std::vector<nauticmiles_t>(m_stations.size(), 0));

        for (size_t i = 0; i < m_stations.size(); ++i) {
            for (size_t j = 0; j < m_stations.size(); ++j) {
                distances[i][j] = geometry::distance(m_stations[i].getLocation(), m_stations[j].getLocation());
            }
        }

        return distances;
    }
};
