#pragma once

#include "geography.h"
#include "path.h"

#include <cmath>
#include <math.h>

namespace geometry {

constexpr long double PI = 3.141592653589793238462643383279502884L;

inline double deg2rad(double degrees)
{
    return degrees*PI/180;
}

inline double rad2deg(double radians)
{
    return radians*180/PI;
}

inline nauticmiles_t distance(const Location &p1, const Location &p2)
{
    double la1 = deg2rad(p1.lat);
    double lo1 = deg2rad(p1.lon);
    double la2 = deg2rad(p2.lat);
    double lo2 = deg2rad(p2.lon);
    return acos(sin(la1) * sin(la2) + cos(la1) * cos(la2) * cos(lo2 - lo1)) * geography::EARTH_RADIUS_NM;
}

inline Location interpolateLocations(const Location &l1, const Location &l2, float x)
{
  // linear interpolation, not exact because lon/lat coordinates cannot be interpolated
  // linearly but good enough for now (on France's map lon/lat can almost be interpreted
  // as xy coordinates)
  return Location(l1.lon + (l2.lon - l1.lon) * x, l1.lat + (l2.lat - l1.lat) * x);
}

inline double cap(const Location &p1, const Location &p2)
{
    double d_lon = deg2rad(p2.lon - p1.lon);
    double lat_1 = deg2rad(p1.lat);
    double lat_2 = deg2rad(p2.lat);
    double y = sin(d_lon) * cos(lat_2);
    double x = cos(lat_1) * sin(lat_2) - sin(lat_1) * cos(lat_2) * cos(d_lon);
    double azimut = rad2deg(atan2(y, x));
    return azimut < 0 ? azimut + 360 : azimut;
}

inline bool isCycle(const Path &path)
{
    if (path[0] != path[path.size() - 1])
        return false;
    for (size_t i = 1; i < path.size()-1; i++) {
        for (size_t j = 0; j < i; j++) {
            if (path[i] == path[j])
                return false;
        }
    }
    return true;
}

}
