#pragma once

#include "assimp/scene.h"

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace GeoUtils
{

  /// <summary>
  /// A couple os functions to convert from 2d points to a 3d assimp mesh
  /// </summary>
  class GeomConvert
  {

  public:
    /// <summary>
    /// A boolean deciding the up axis as z
    /// </summary>
    static bool zUp;
    static float texCoordScale;

    static glm::vec3 upNormal();
    static glm::vec3 posFromLoc(double lon, double lat, double height);

    /// <summary>
    /// Given an enclosed loop of 2d points defining a polygon the function returns a 3d mesh
    /// with the polygon as it's base and top extruded to the value of the given height
    /// <summary>
    static aiMesh *extrude2dMesh(const std::vector<glm::vec2> &baseVertices, float height, int featureId);

    /// <summary>`
    /// Given a list of points as a line, creates a flat mesh along the line of the given width
    /// </summary>
    static aiMesh *meshFromLine(const std::vector<glm::vec2> &line, float width, int featureId);
  };

} // namespace GeoUtils