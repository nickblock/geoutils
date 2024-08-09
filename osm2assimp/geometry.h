#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <span>
#include <vector>

class aiMesh;

namespace GeoUtils {

// Geomtry class handles creation of 3d objects from osm data

class Geometry {

public:
  using TVertIdx = uint32_t;
  using Tri = std::array<TVertIdx, 3>;

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

  using Face = std::vector<TVertIdx>;
  using FaceList = std::vector<Face>;

  struct Data3D {
    std::vector<glm::vec3> mVertices;
    std::vector<glm::vec3> mNormals;
    std::vector<glm::vec3> mTexCoords;
    FaceList mFaces;

    aiMesh *toMesh() const;
  };

  struct DataFlat {
    std::vector<glm::vec2> mVertices;
    FaceList mFaces;
  };

  const DataFlat &getFootprint() { return mDataFlat; }

  aiMesh *simpleMesh() const { return mData.toMesh(); }

protected:
  Geometry() = default;

  Data3D mData;

  DataFlat mDataFlat;

public:
  // given 2d polygon,
  // produce list of triangular faces
  static Geometry::DataFlat triangulate(const std::span<glm::vec2> &vertices);

  // print polygon to svg for debug purposes
  static void writeSvg(const DataFlat &data, const std::filesystem::path &file);

  friend class Ground;
};

using Line = std::array<glm::vec2, 2>;

bool lineIntersects2d(const Line &l0, const Line &l1,
                      glm::vec2 *intersection = nullptr);

bool pointOnLine(const Line &line, const glm::vec2 &point);

} // namespace GeoUtils