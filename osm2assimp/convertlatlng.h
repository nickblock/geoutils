#pragma once

#include <osmium/geom/coordinates.hpp>

namespace EngineBlock {

  class ConvertLatLngToCoords {

  public:

    static osmium::Location RefPoint;
    static bool             UseCenterEarthFixed;
      
    static osmium::geom::Coordinates cef(const osmium::Location& location);
    static osmium::geom::Coordinates osm(const osmium::Location& location);

    int epsg() const noexcept {
        return 1111;
    }
  
    static osmium::geom::Coordinates to_coords(const osmium::Location& location);
  };


}