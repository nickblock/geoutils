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

// factory methods take 2d points as input and produce various types of 3d
// objects. Geometry always has 2d data, but may need extrudeToMesh to be called
// to create 3d counterpart

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

  static Geometry meshFromJunction(const glm::vec2 &center,
                                   const std::vector<glm::vec2> &offroads,
                                   float width);

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
    Data3D extrude3DFromFlat(float height, int featureId = 0);
  };

  //extrude3DFromFlat produces a 3d object of height, with the footprint of 
  // the 2d flat poly.
  // Calling extrude3DFromFlat with zero height, prodices a flat polygon in 3d space
  Data3D &extrude3DFromFlat(float height, int featureId = 0) {
    mData = mDataFlat.extrude3DFromFlat(height, featureId);
    return mData;
  }

  DataFlat &getFootprint() { return mDataFlat; }

  aiMesh *toMesh() const {
    if (mData.mVertices.size() == 0) {
      std::cout << "NO 3d data, perhaps call extrude first?" << std::endl;
      return nullptr;
    }
    return mData.toMesh();
  }

protected:
  Geometry() = default;

  Data3D mData;

  DataFlat mDataFlat;

  int mFeatureId = 0;
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