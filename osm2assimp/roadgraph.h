#include <map>
#include <osmium/osm/way.hpp>
#include <set>
#include <vector>

namespace GeoUtils {

class RoadGraph {

public:
  ~RoadGraph();
  using Road = std::vector<osmium::NodeRef>;
  using Junction = std::tuple<osmium::NodeRef, std::vector<size_t>>;

  void addRoad(const osmium::Way &way);

  void graph();

  size_t numRoads() const;
  const Road *getRoad(size_t idx) const;

protected:
  Road splitRoad(size_t roadIdx, const osmium::NodeRef &ref);

  void joinRoadsAtNode(const osmium::NodeRef &node);
  void setNodeRefToRoad(const osmium::NodeRef &node, size_t roadIdx,
                        size_t fromIdx);

  std::vector<Road> mRoads;
  std::map<osmium::NodeRef, std::set<size_t>> mNodeToWay;
};
} // namespace GeoUtils