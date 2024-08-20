#pragma once

#include <filesystem>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <optional>
#include <span>
#include <vector>

class aiMesh;

namespace GeoUtils {

using TVertIdx = uint32_t;
using Face = std::vector<TVertIdx>;
using FaceList = std::vector<Face>;
using Edge = std::array<TVertIdx, 2>;

// Geomtry class handles creation of 3d objects from osm data

class Geometry {

public:
  /// <summary>
  /// Given an enclosed loop of 2d points defining a polygon the function
  /// returns a 3d mesh with the polygon as it's base and top extruded to the
  /// value of the given height. <summary>
  static Geometry extrude2dMesh(const std::vector<glm::vec2> &baseVertices,
                                float height, int featureId = 0);

  /// <summary>`
  /// Given a list of points as a line, creates a flat mesh along the line of
  /// the given width.
  /// </summary>
  static Geometry meshFromLine(const std::vector<glm::vec2> &line, float width,
                               int featureId = 0);

  /// <summary>
  /// A boolean deciding the up axis as z
  /// </summary>
  static bool zUp;
  static float texCoordScale;

  static glm::vec3 upNormal();
  static glm::vec3 posFromLoc(double lon, double lat, double height);
  static glm::vec3 fromGround(const glm::vec2 &groundCoords);

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

    DataFlat &operator+(const DataFlat &other);
  };

  DataFlat &getFootprint() { return mDataFlat; }

  aiMesh *simpleMesh() const { return mData.toMesh(); }

protected:
  Geometry() = default;

  Data3D mData;

  DataFlat mDataFlat;
};

using Line = std::array<glm::vec2, 2>;

std::tuple<glm::vec2, bool> lineIntersects2d(const Line &l0, const Line &l1);

bool pointOnLine(const Line &line, const glm::vec2 &point);

template <typename GLMVEC> bool isSame(const GLMVEC &a, const GLMVEC &b) {

  for (int i = 0; i < a.length(); i++) {
    float diff = fabs(a[i] - b[i]);
    if (diff > glm::epsilon<float>()) {
      return false;
    }
  }
  return true;
}

} // namespace GeoUtils