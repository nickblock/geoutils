#include <map>
#include <osmium/osm/way.hpp>
#include <set>
#include <vector>

namespace GeoUtils {

class RoadGraph {

public:
  ~RoadGraph();
  using Road = std::vector<osmium::NodeRef>;

  struct Junction {
    osmium::NodeRef center;
    std::vector<osmium::NodeRef>
        offRoads; // the next nodeRef of each road out from the junction
  };

  void addRoad(const osmium::Way &way);

  void graph();

  size_t numRoads() const;
  const Road *getRoad(size_t idx) const;

protected:
  Road splitRoad(size_t roadIdx, const osmium::NodeRef &ref);

  void joinRoadsAtNode(const osmium::NodeRef &node);
  void setNodeRefToRoad(const osmium::NodeRef &node, size_t roadIdx,
                        size_t fromIdx);

  void joinRoads();
  void makeJunctions();

  std::vector<Road> mRoads;
  std::vector<Junction> mJunctions;

  // map each nodeRef to 1 or more roads which share it
  std::map<osmium::NodeRef, std::set<size_t>> mNodeToWay;
};
} // namespace GeoUtils