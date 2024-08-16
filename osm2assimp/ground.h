#pragma once

#include "geometry.h"
#include "glm/vec2.hpp"
#include "osmfeature.h"
#include "triangulate.h"
#include "utils.h"
#include <filesystem>
#include <map>
#include <vector>

class aiMesh;

namespace GeoUtils {
class RoadNetwork;

class Ground {
public:
  enum GroundTypes { Building, Road, Water };
  Ground(const std::vector<glm::vec2> &);
  virtual ~Ground();

  void addFootPrint(const Triangulate::Data &footprint, GroundTypes type);

  aiMesh *getMesh();

  void writeSvg(const std::filesystem::path &path);

protected:
  using BoxPoly = std::tuple<BBox, std::vector<glm::vec2>>;
  static constexpr int Box = 0;
  static constexpr int Poly = 1;

  std::vector<glm::vec2> mExtents;
  std::vector<glm::vec2> mGroundPoints;
  std::vector<Edge> mEdges;

  Geometry::DataFlat mDataFlat;

  std::unordered_map<std::size_t, int> mPointTypes;

  int mAdded = 0;

  BBox mBBox;

  std::unique_ptr<RoadNetwork> mRoadNetwork;
};
} // namespace GeoUtils