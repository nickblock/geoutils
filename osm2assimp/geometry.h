#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <span>
#include <vector>

class aiMesh;

namespace GeoUtils {

// Geomtry class handles creation of 3d objects from osm data

class Geometry {

public:
  /// <summary>
  /// Given an enclosed loop of 2d points defining a polygon the function
  /// returns a 3d mesh with the polygon as it's base and top extruded to the
  /// value of the given height. <summary>
  static Geometry extrude2dMesh(const std::vector<glm::vec2> &baseVertices,
                                float height, int featureId);

  /// <summary>`
  /// Given a list of points as a line, creates a flat mesh along the line of
  /// the given width.
  /// </summary>
  static Geometry meshFromLine(const std::vector<glm::vec2> &line, float width,
                               int featureId);

  /// <summary>
  /// A boolean deciding the up axis as z
  /// </summary>
  static bool zUp;
  static float texCoordScale;

  static glm::vec3 upNormal();
  static glm::vec3 posFromLoc(double lon, double lat, double height);
  static glm::vec3 fromGround(const glm::vec2 &groundCoords);

  // given mesh vertices extract ground points by looking at height value (z or
  // y) beig zero
  //  returns vector of double to be used in delaunator.hpp
  static std::vector<double> getFootprint(std::span<const glm::vec3> vertices);

  aiMesh *simpleMesh() { return mData.toMesh(); }

protected:
  Geometry() = default;

  using Face = std::vector<int>;
  using FaceList = std::vector<Face>;

  struct Data {
    std::vector<glm::vec3> mVertices;
    std::vector<glm::vec3> mNormals;
    std::vector<glm::vec3> mTexCoords;
    FaceList mFaces;

    aiMesh *toMesh() const;
  };

  Data mData;
  std::vector<glm::vec2> mFootPrint;

public:
  // given 2d polygon (given vec3 is assumed to flat on in one dimension),
  // produce list of triangular faces
  static FaceList triangulate(const std::span<glm::vec3> &vertices);

  // print polygon to svg for debug purposes
  static void writeSvg(const FaceList &faces,
                       const std::vector<glm::vec3> &vertices,
                       const std::filesystem::path &file);
};

} // namespace GeoUtils