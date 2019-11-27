#pragma once

#include <osmium/geom/coordinates.hpp>

namespace EngineBlock {

  class CenterEarthFixedConvert {

  public:

    static osmium::Location refPoint;
      
    osmium::geom::Coordinates operator()(const osmium::Location& location) const;

    int epsg() const noexcept {
        return 1111;
    }
  
    static osmium::geom::Coordinates to_coords(const osmium::Location& location);
  };


}