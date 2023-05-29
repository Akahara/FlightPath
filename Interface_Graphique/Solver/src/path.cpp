#include "path.h"

#include "geometry.h"

nauticmiles_t Path::length() const
{
    nauticmiles_t length = 0;
    for (size_t i = 1; i < size(); i++)
        length += geometry::distance(m_stations[i]->getLocation(), m_stations[i - 1]->getLocation());
    return length;
}
