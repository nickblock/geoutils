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

void RoadGraph::joinRoadsAtNode(const osmium::NodeRef &node) {

  auto setIt = mNodeToWay[node].begin();

  auto roadIdx0 = *setIt;
  auto roadIdx1 = *(++setIt);

  auto &road0 = mRoads[roadIdx0];
  auto &road1 = mRoads[roadIdx1];

  int pos0 = getPosition(road0, node);
  int pos1 = getPosition(road1, node);

  if (pos0 == End && pos1 == Begin) {
    road0.insert(road0.end(), road1.begin() + 1, road1.end());
    for (auto &node : road1) {
      setNodeRefToRoad(node, roadIdx1, roadIdx0);
    }
    road1.clear();

  } else if (pos0 == End && pos1 == End) {
    std::reverse(road1.begin(), road1.end());
    road0.insert(road0.end(), road1.begin() + 1, road1.end());
    for (auto &node : road1) {
      setNodeRefToRoad(node, roadIdx1, roadIdx0);
    }
    road1.clear();
  } else if (pos0 == Begin && pos1 == Begin) {
    std::reverse(road0.begin(), road0.end());
    road0.insert(road0.end(), road1.begin() + 1, road1.end());
    for (auto &node : road1) {
      setNodeRefToRoad(node, roadIdx1, roadIdx0);
    }
    road1.clear();
  } else if (pos0 == Begin && pos1 == End) {

    road1.insert(road1.end(), road0.begin() + 1, road0.end());
    for (auto &node : road0) {
      setNodeRefToRoad(node, roadIdx0, roadIdx1);
    }
    road0.clear();
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
void RoadGraph::graph() {

  std::cout << std::format("Num Roads begin {}", numRoads()) << std::endl;
  for (auto &nodeRoad : mNodeToWay) {
    if (nodeRoad.second.size() == 2) {
      // join roads
      auto node = nodeRoad.first;

      joinRoadsAtNode(node);

    } else if (nodeRoad.second.size() > 2) {
      // junction
    }
  }

  std::cout << std::format("Num Roads after {}", numRoads()) << std::endl;
}
} // namespace GeoUtils