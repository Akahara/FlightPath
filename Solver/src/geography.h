#pragma once

#include <cmath>

typedef double nauticmiles_t;
typedef double km_t;
typedef unsigned char cardinal_t;

// Cardinals, can be considered a bitwise enum
namespace Card {

constexpr int N_offset = 0, S_offset = 1, E_offset = 2, W_offset = 3;
constexpr cardinal_t
  N = 1<<N_offset,
  S = 1<<S_offset,
  E = 1<<E_offset,
  W = 1<<W_offset;
constexpr cardinal_t NE = N | E, NW = N | W, SE = S | E, SW = S | W;

}

// A (longitude latitude) pair, in degrees
// both .lon and .longitude can be used to refer to the location's longitude
// the same goes for latitude with .lat and .latitude
// for example: 3N5W is (lon=-5 lat=3)
// 
// longitude is in range -180..180
// latitude is in range -90..90
// Both values are clamped in the constructor, when modifying lon/lat values you
// are responsible to ensure these ranges are respected, you may create a new
// instance to make sure they are.
// 
// Prefer using convenience constructors fromNECoordinates and fromDegreeMinuteSecond
// rather than constructing the object by hand.
struct Location {
    union { double lon; double longitude; };
    union { double lat; double latitude;  };

    Location(double longitude, double latitude)
    {
        lon = std::fmod(longitude, 360.);
        if (lon <= -180) lon += 360; else if (lon > +180) lon -= 360;
        lat = std::fmod(latitude, 360.);
        if (lat <= -180) lat += 360; else if (lat > +180) lat -= 360;
        if (lat <= -90) { lon = -lon; lat += 180; } else if (lat > +90) { lat -= 180; lat = -lat; }
    }

    Location()
        : lon(0), lat(0)
    {
    }

    /*
     * Degrees south and west can be passed too, use Card::XX to specify the cardinality of your coordinates.
     * Alternatively negative degrees north will be interpreted as degrees south.
     */
    static Location fromNECoordinates(double degreesNorth, double degreesEast, cardinal_t cardinals=Card::NE)
    {
        Location l{ degreesEast, degreesNorth };
        if (cardinals & Card::W) l.lon = -l.lon;
        if (cardinals & Card::S) l.lat = -l.lat;
        return l;
    }

    static Location fromDegreeMinuteSecond(int dn, int mn, int sn, int de, int me, int se, cardinal_t cardinals)
    {
        Location l = fromNECoordinates(dn + mn/60. + sn/3600., de + me/60. + se/3600.);
        if (cardinals & Card::W) l.lon = -l.lon;
        if (cardinals & Card::S) l.lat = -l.lat;
        return l;
    }

    bool operator==(const Location &other) const { return lon == other.lon && lat == other.lat; }
    bool operator!=(const Location &other) const { return lon != other.lon || lat != other.lat; }
};

namespace geography {

constexpr double NAUTICMILES_TO_KILOMETERS_FACTOR = 1.852;
constexpr nauticmiles_t EARTH_RADIUS_NM = 3443.9308855292;
constexpr km_t EARTH_RADIUS_KM = 6371;

inline km_t nauticmiles2km(nauticmiles_t nautics)
{
    return nautics * NAUTICMILES_TO_KILOMETERS_FACTOR;
}

inline nauticmiles_t km2nauticmiles(km_t kilometers)
{
    return kilometers / NAUTICMILES_TO_KILOMETERS_FACTOR;
}

}
