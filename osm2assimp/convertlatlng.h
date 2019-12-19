#pragma once

#include <osmium/geom/coordinates.hpp>
#include <osmium/osm/location.hpp>


namespace GeoUtils {

/// <summary>
/// A class providing two alternative methods for converting WSG coordinated to euclidean ones
/// </summary>
class ConvertLatLngToCoords {

public:
  static bool             UseCenterEarthFixed;

  /// <summary>
  /// Set the ref point. UseCenterEarthFixed should be set before setting this, as the way it is used is dependant 
  /// on the algorithm used. 
  /// </summary>
  static void setRefPoint(const osmium::Location& loc);
    
  /// <summary>
  /// Convert coords using Center Earth Fixed
  /// The maths can be found in eigenconversion.cpp
  /// </summary>
  static osmium::geom::Coordinates cef(const osmium::Location& location);

  /// <summary>
  /// convert coords using osmium defined mercator projection
  /// </summary>
  static osmium::geom::Coordinates osm(const osmium::Location& location);

  int epsg() const noexcept {
      return 1111;
  }

  static osmium::geom::Coordinates to_coords(const osmium::Location& location);

private:
  /// <summary>
  /// Both algorithms rely on a nearby reference point to overcome margin of error
  /// <summary>
  static std::tuple<double, double> RefPoint;
};

}