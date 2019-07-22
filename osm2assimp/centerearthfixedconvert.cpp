
#include "centerearthfixedconvert.h"
#include "eigenconversion.h"

namespace EngineBlock {
  
  osmium::Location CenterEarthFixedConvert::refPoint(-1.0, -1.0);

  osmium::geom::Coordinates CenterEarthFixedConvert::operator()(const osmium::Location& location) const {

      auto localCoordinates = LLAtoNED(Eigen::Vector3d{refPoint.lat(), refPoint.lon(), 0.0}, Eigen::Vector3d{location.lat(), location.lon(), 0.0}); 
    
      return osmium::geom::Coordinates{localCoordinates.y(), localCoordinates.x()};
  } 

  osmium::geom::Coordinates CenterEarthFixedConvert::to_coords(const osmium::Location& location)
  {
    CenterEarthFixedConvert cef;
    return cef(location);
  }
}