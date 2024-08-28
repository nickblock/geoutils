#pragma once

#include <format>
#include <iostream>
#include <map>
#include <osmium/osm/way.hpp>
#include <set>
#include <span>
#include <vector>

namespace GeoUtils {

template <typename NodeType> class RoadGraph {

public:
  ~RoadGraph() = default;

  using Road = std::vector<NodeType>;

  struct Junction {
    NodeType center;
    std::vector<NodeType>
        offRoads; // the next nodeRef of each road out from the junction
  };

  void addRoad(const std::span<NodeType> &way) {

    size_t roadIdx = mRoads.size();
    Road road(way.size());
    for (int i = 0; i < way.size(); i++) {
      road[i] = way[i];
      mNodeToWay[road[i]].insert(roadIdx);
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
      if (road.size()) {
        num++;
      }
    }
    return num;
  }
  const Road *getRoad(size_t idx) const {
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

  size_t numJunctions() { return mJunctions.size(); }

  const Junction *getJunctions(size_t idx) { return &mJunctions[idx]; }

protected:
  std::vector<Road> mRoads;
  std::vector<Junction> mJunctions;

  // map each nodeRef to 1 or more roads which share it
  std::map<NodeType, std::set<size_t>> mNodeToWay;

  enum NodePosition { Begin, Middle, End, None };

  NodePosition getPosition(const RoadGraph::Road &road, const NodeType &node) {
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

  void setNodeRefToRoad(const NodeType &node, size_t roadIdx, size_t fromIdx) {
    auto &set = mNodeToWay[node];
    set.erase(fromIdx);
    set.insert(roadIdx);
  }

  Road splitRoad(size_t roadIdx, const NodeType &node) {
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
};
} // namespace GeoUtils