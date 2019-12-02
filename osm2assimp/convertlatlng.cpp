  
#include "convertlatlng.h"
#include "eigenconversion.h"

#include <osmium/geom/mercator_projection.hpp>

namespace GeoUtils {

osmium::Location ConvertLatLngToCoords::RefPoint(-1.0, -1.0);
bool ConvertLatLngToCoords::UseCenterEarthFixed = false;  

osmium::geom::Coordinates ConvertLatLngToCoords::cef(const osmium::Location& location) {

    auto localCoordinates = LLAtoNED(Eigen::Vector3d{RefPoint.lat(), RefPoint.lon(), 0.0}, Eigen::Vector3d{location.lat(), location.lon(), 0.0}); 
  
    return osmium::geom::Coordinates{localCoordinates.y(), localCoordinates.x()};
} 
osmium::geom::Coordinates ConvertLatLngToCoords::osm(const osmium::Location& location) {

    return osmium::geom::lonlat_to_mercator(osmium::Location(location.lon() - RefPoint.lon(), location.lat() - RefPoint.lat()));
}

osmium::geom::Coordinates ConvertLatLngToCoords::to_coords(const osmium::Location& location)
{
  if(UseCenterEarthFixed) {
    return cef(location);
  }
  else {
    return osm(location);
  }
}

}