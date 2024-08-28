#include "roadgraph.h"
#include <format>
#include <iostream>

namespace GeoUtils {

RoadGraph::~RoadGraph() = default;

void RoadGraph::addRoad(const osmium::Way &way) {

  size_t roadIdx = mRoads.size();
  Road road(way.nodes().size());
  for (int i = 0; i < way.nodes().size(); i++) {
    road[i] = way.nodes()[i];
    mNodeToWay[road[i]].insert(roadIdx);
  }
  mRoads.push_back(road);
}

enum NodePosition { Begin, Middle, End, None };

NodePosition getPosition(const RoadGraph::Road &road,
                         const osmium::NodeRef &node) {
  for (int i = 0; i < road.size(); i++) {
    if (road[i] == node) {
      if (i == 0) {
        return Begin;
      } else if (i == road.size() - 1) {
        return End;
      } else {
        return Middle;
      }
    }
  }
  return None;
}

void RoadGraph::setNodeRefToRoad(const osmium::NodeRef &node, size_t roadIdx,
                                 size_t fromIdx) {
  auto &set = mNodeToWay[node];
  set.erase(fromIdx);
  set.insert(roadIdx);
}

RoadGraph::Road RoadGraph::splitRoad(size_t roadIdx,
                                     const osmium::NodeRef &node) {
  auto &road = mRoads[roadIdx];

  size_t splitPoint = road.size();
  for (int i = 0; i < road.size(); i++) {
    if (road[i] == node) {
      splitPoint = i;
      break;
    }
  }

  assert(splitPoint != road.size());

  Road newRoad;
  newRoad.insert(newRoad.begin(), road.begin() + splitPoint, road.end());
  road.erase(road.begin() + splitPoint + 1, road.end());

  auto newRoadIdx = mRoads.size();

  for (auto it = newRoad.begin() + 1; it != newRoad.end(); ++it) {
    setNodeRefToRoad(*it, newRoadIdx, roadIdx);
  }
  mNodeToWay[node].insert(newRoadIdx);
  mRoads.emplace_back(std::move(newRoad));

  return newRoad;
}

void RoadGraph::joinRoadsAtNode(const osmium::NodeRef &node) {

  auto &set = mNodeToWay[node];
  auto setIt = set.begin();

  auto roadIdx0 = *setIt;
  auto roadIdx1 = *(++setIt);

  auto &road0 = mRoads[roadIdx0];
  auto &road1 = mRoads[roadIdx1];

  int pos0 = getPosition(road0, node);
  int pos1 = getPosition(road1, node);

  if (pos0 == End && pos1 == Begin) {
    road0.insert(road0.end(), road1.begin() + 1, road1.end());
    for (auto &node : road1) {
      setNodeRefToRoad(node, roadIdx0, roadIdx1);
    }
    road1.clear();

  } else if (pos0 == End && pos1 == End) {
    std::reverse(road1.begin(), road1.end());
    road0.insert(road0.end(), road1.begin() + 1, road1.end());
    for (auto &node : road1) {
      setNodeRefToRoad(node, roadIdx0, roadIdx1);
    }
    road1.clear();
  } else if (pos0 == Begin && pos1 == Begin) {
    std::reverse(road0.begin(), road0.end());
    road0.insert(road0.end(), road1.begin() + 1, road1.end());
    for (auto &node : road1) {
      setNodeRefToRoad(node, roadIdx0, roadIdx1);
    }
    road1.clear();
  } else if (pos0 == Begin && pos1 == End) {

    road1.insert(road1.end(), road0.begin() + 1, road0.end());
    for (auto &node : road0) {
      setNodeRefToRoad(node, roadIdx1, roadIdx0);
    }
    road0.clear();
  } else if (pos1 == Middle || pos0 == Middle) {
    if (pos0 == Middle) {
      splitRoad(roadIdx0, node);
    }
    if (pos1 == Middle) {
      splitRoad(roadIdx1, node);
    }
  } else {
    assert(false);
  }
}

size_t RoadGraph::numRoads() const {
  size_t num = 0;
  for (auto &road : mRoads) {
    if (road.size()) {
      num++;
    }
  }
  return num;
}
const RoadGraph::Road *RoadGraph::getRoad(size_t idx) const {
  size_t search = 0;
  size_t actual = 0;
  while (actual <= idx) {
    while (mRoads[search].size() == 0) {
      search++;
    }
    if (actual == idx) {
      return &mRoads[search];
    }
    actual++;
    search++;
  }
  return nullptr;
}

void RoadGraph::joinRoads() {
  for (auto &nodeRoad : mNodeToWay) {
    if (nodeRoad.second.size() == 2) {
      // join roads
      auto node = nodeRoad.first;

      joinRoadsAtNode(node);
    }
  }
}
void RoadGraph::makeJunctions() {

  for (auto &nodeRoad : mNodeToWay) {
    assert(nodeRoad.second.size() != 2);

    if (nodeRoad.second.size() > 2) {
      // junction

      // check if the junction is found in middle of a road,
      // in which case split it.
      for (auto &roadIdx : nodeRoad.second) {
        auto &road = mRoads[roadIdx];
        auto pos = getPosition(road, nodeRoad.first);
        if (pos == Middle) {
          splitRoad(roadIdx, nodeRoad.first);
        }
      }

      Junction junction;
      junction.center = nodeRoad.first;

      for (auto &roadIdx : nodeRoad.second) {
        auto &road = mRoads[roadIdx];
        auto jPos = getPosition(road, junction.center);

        if (jPos == Begin) {
          junction.offRoads.push_back(road[1]);
        } else if (jPos == End) {
          junction.offRoads.push_back(road[road.size() - 2]);
        } else {
          assert(false);
        }
      }
      mJunctions.emplace_back(std::move(junction));
    }
  }
}
void RoadGraph::graph() {

  std::cout << std::format("Num Roads begin {}", numRoads()) << std::endl;
  joinRoads();

  std::cout << std::format("Num Roads after {}", numRoads()) << std::endl;
  // makeJunctions();

  std::cout << std::format("Num Junctions {}", mJunctions.size()) << std::endl;
}

} // namespace GeoUtils