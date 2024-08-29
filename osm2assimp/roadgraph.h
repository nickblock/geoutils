#pragma once

#include <format>
#include <iostream>
#include <map>
#include <osmium/osm/way.hpp>
#include <set>
#include <span>
#include <vector>

namespace GeoUtils {

constexpr size_t kInvalidIdx = (size_t)-1;

template <typename NodeType> class RoadGraph {

public:
  ~RoadGraph() = default;

  struct Road {
    Road() = default;
    Road(size_t size) : nodes(size) {}
    std::vector<NodeType> nodes;
    std::array<size_t, 2> junctions = {kInvalidIdx, kInvalidIdx};
  };

  struct Junction {
    NodeType center;
    std::vector<NodeType>
        offRoads; // the next nodeRef of each road out from the junction
  };

  void addRoad(const std::span<NodeType> &way) {

    size_t roadIdx = mRoads.size();
    Road road(way.size());
    for (int i = 0; i < way.size(); i++) {
      road.nodes[i] = way[i];
      mNodeToWay[road.nodes[i]].insert(roadIdx);
    }
    mRoads.push_back(road);
  }
  void graph() {

    std::cout << std::format("Num Roads begin {}", numRoads()) << std::endl;
    joinRoads();

    std::cout << std::format("Num Roads after {}", numRoads()) << std::endl;
    makeJunctions();

    std::cout << std::format("Num Junctions {}", mJunctions.size())
              << std::endl;
  }
  size_t numRoads() const {
    size_t num = 0;
    for (auto &road : mRoads) {
      if (road.nodes.size()) {
        num++;
      }
    }
    return num;
  }
  const Road *getRoad(size_t idx) const {
    size_t search = 0;
    size_t actual = 0;
    while (actual <= idx) {
      while (mRoads[search].nodes.size() == 0) {
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

  size_t numJunctions() { return mJunctions.size(); }

  const Junction *getJunctions(size_t idx) { return &mJunctions[idx]; }

protected:
  std::vector<Road> mRoads;
  std::vector<Junction> mJunctions;

  // map each nodeRef to 1 or more roads which share it
  std::map<NodeType, std::set<size_t>> mNodeToWay;

  enum NodePosition { Begin, Middle, End, None };

  NodePosition getPosition(const RoadGraph::Road &road, const NodeType &node) {
    for (int i = 0; i < road.nodes.size(); i++) {
      if (road.nodes[i] == node) {
        if (i == 0) {
          return Begin;
        } else if (i == road.nodes.size() - 1) {
          return End;
        } else {
          return Middle;
        }
      }
    }
    return None;
  }

  void setNodeRefToRoad(const NodeType &node, size_t roadIdx, size_t fromIdx) {
    auto &set = mNodeToWay[node];
    set.erase(fromIdx);
    set.insert(roadIdx);
  }

  Road splitRoad(size_t roadIdx, const NodeType &node) {
    auto &road = mRoads[roadIdx];

    size_t splitPoint = road.nodes.size();
    for (int i = 0; i < road.nodes.size(); i++) {
      if (road.nodes[i] == node) {
        splitPoint = i;
        break;
      }
    }

    assert(splitPoint != road.nodes.size());

    Road newRoad;
    newRoad.nodes.insert(newRoad.nodes.begin(), road.nodes.begin() + splitPoint,
                         road.nodes.end());
    road.nodes.erase(road.nodes.begin() + splitPoint + 1, road.nodes.end());

    auto newRoadIdx = mRoads.size();

    for (auto it = newRoad.nodes.begin() + 1; it != newRoad.nodes.end(); ++it) {
      setNodeRefToRoad(*it, newRoadIdx, roadIdx);
    }
    mNodeToWay[node].insert(newRoadIdx);
    mRoads.emplace_back(std::move(newRoad));

    return newRoad;
  }

  // addition gets added to end of source nodeways are
  // updated to point to source
  // addition is cleared
  void appendRoad(size_t sourceIdx, size_t addIdx) {
    auto &source = mRoads[sourceIdx];
    auto &addition = mRoads[addIdx];
    source.nodes.insert(source.nodes.end(), addition.nodes.begin() + 1,
                        addition.nodes.end());
    for (auto &node : addition.nodes) {
      setNodeRefToRoad(node, sourceIdx, addIdx);
    }
    addition.nodes.clear();
  }

  void joinRoadsAtNode(const NodeType &node) {

    auto &set = mNodeToWay[node];
    auto setIt = set.begin();

    auto roadIdx0 = *setIt;
    auto roadIdx1 = *(++setIt);

    auto &road0 = mRoads[roadIdx0];
    auto &road1 = mRoads[roadIdx1];

    int pos0 = getPosition(road0, node);
    int pos1 = getPosition(road1, node);

    if (pos0 == End && pos1 == Begin) {
      appendRoad(roadIdx0, roadIdx1);
    } else if (pos0 == End && pos1 == End) {
      std::reverse(road1.nodes.begin(), road1.nodes.end());
      appendRoad(roadIdx0, roadIdx1);
    } else if (pos0 == Begin && pos1 == Begin) {
      std::reverse(road0.nodes.begin(), road0.nodes.end());
      appendRoad(roadIdx0, roadIdx1);
    } else if (pos0 == Begin && pos1 == End) {
      appendRoad(roadIdx1, roadIdx0);
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

  void joinRoads() {
    for (auto &nodeRoad : mNodeToWay) {
      if (nodeRoad.second.size() == 2) {
        // join roads
        auto node = nodeRoad.first;

        joinRoadsAtNode(node);
      }
    }
  }
  void makeJunctions() {

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

        auto junctionIdx = mJunctions.size();

        for (auto &roadIdx : nodeRoad.second) {
          auto &road = mRoads[roadIdx];
          auto jPos = getPosition(road, junction.center);

          if (jPos == Begin) {
            junction.offRoads.push_back(road.nodes[1]);

            assert(road.junctions[0] == kInvalidIdx);
            road.junctions[0] = junctionIdx;
          } else if (jPos == End) {
            junction.offRoads.push_back(road.nodes[road.nodes.size() - 2]);
            assert(road.junctions[1] == kInvalidIdx);
            road.junctions[1] = junctionIdx;
          } else {
            assert(false);
          }
        }
        mJunctions.emplace_back(std::move(junction));
      }
    }
  }
};
} // namespace GeoUtils