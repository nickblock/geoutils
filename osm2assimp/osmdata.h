#ifndef OSM_DATA_H
#define OSM_DATA_H

#include <string>
#include <map>
#include <vector>
#include "osmfeature.h"
#include <osmium/handler.hpp>

class AssimpConstruct;
class aiNode;

namespace xmlpp {
  class Node;
}

/// <summary>
/// A class that handles the reading of ways and nodes from an OSM file
/// Each node or way is converted to an OSMFeature, which is later converted to an Assimp mesh
/// <summary>
class OSMDataImport : public osmium::handler::Handler {

public: 
  OSMDataImport(AssimpConstruct& ac, const osmium::Box& extents, int filter = 0xffffffff );

  void way(const osmium::Way& way);

  void node(const osmium::Node& node);

  /// <summary>
  /// Optionally a parent node can be assigned, which all the meshes 
  /// converted from OSMFeatures will be parented to.
  /// </summary>
  void setParentAINode(aiNode* node);

  int exportCount() {
    return mCount;
  }

private:

  void process(const OSMFeature& feature);

  int mCount;
  int mFilter;
  const osmium::Box& mExtents;
  WorldCoord mRefPoint;
  std::map<std::string, glm::vec3>  mMatColors;
  std::vector<OSMFeature> mFeatures;
  AssimpConstruct& mAssimpConstruct;
  aiNode* mParentNode;
};
#endif