#pragma once

#include "glm/vec2.hpp"
#include "osmfeature.h"
#include "utils.h"
#include <filesystem>
#include <map>
#include <vector>

class aiMesh;

namespace GeoUtils {

using Point = std::pair<double, double>;

class Ground {
public:
  Ground(const std::vector<glm::vec2> &);

  void addFootPrint(const std::vector<double> &points, int type);

  aiMesh *getMesh();

  void writeSvg(const std::filesystem::path &path, float scale);

protected:
  std::size_t hashKey(Point point) const;

  // from the delaunay tris remove all tris that encompass a road or building,
  // leaving the inbetween tris; the ground
  std::vector<double> findGroundTris(const std::vector<double> &delaunayTris);

  using BoxPoly = std::tuple<BBox, std::vector<glm::vec2>>;
  static constexpr int Box = 0;
  static constexpr int Poly = 1;

  static constexpr int kHashSize = 1000000;

  std::vector<glm::vec2> mExtents;
  std::vector<double> mGroundPoints;

  std::unordered_map<std::size_t, int> mPointTypes;

  int mAdded = 0;

  BBox mBBox;
};
} // namespace GeoUtils