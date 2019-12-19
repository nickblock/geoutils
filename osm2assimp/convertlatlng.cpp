  
#include "convertlatlng.h"
#include "eigenconversion.h"

#include <osmium/geom/mercator_projection.hpp>

namespace GeoUtils {

std::tuple<double, double> ConvertLatLngToCoords::RefPoint(-1.0, -1.0);
bool ConvertLatLngToCoords::UseCenterEarthFixed = false;  

void ConvertLatLngToCoords::setRefPoint(const osmium::Location& loc) {
  if(UseCenterEarthFixed) {
    RefPoint = {loc.lon(), loc.lat()};
  }
  else {
    auto coord = osmium::geom::lonlat_to_mercator(loc);
    RefPoint = {coord.x, coord.y};    
  }
}
osmium::geom::Coordinates ConvertLatLngToCoords::cef(const osmium::Location& location) {

    auto localCoordinates = LLAtoNED(Eigen::Vector3d{std::get<1>(RefPoint), std::get<0>(RefPoint), 0.0}, Eigen::Vector3d{location.lat(), location.lon(), 0.0}); 
  
    return osmium::geom::Coordinates{localCoordinates.y(), localCoordinates.x()};
} 
osmium::geom::Coordinates ConvertLatLngToCoords::osm(const osmium::Location& location) {

    //the mercator projection gives very different results for different regions of the planet, which is awy the refpoint
    // is negated after the calcualtion is done
    auto coord = osmium::geom::lonlat_to_mercator(osmium::Location(location.lon(), location.lat()));

    return osmium::geom::Coordinates(coord.x - std::get<0>(RefPoint), coord.y - std::get<1>(RefPoint));
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