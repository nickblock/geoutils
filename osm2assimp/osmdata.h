#pragma once

#include <string>
#include <map>
#include <vector>
#include "osmfeature.h"
#include "viewfilter.h"
#include <osmium/handler.hpp>

class aiNode;

namespace xmlpp {
  class Node;
}

namespace GeoUtils {

class AssimpConstruct;

/// <summary>
/// A class that handles the reading of ways and nodes from an OSM file
/// Each node or way is converted to an OSMFeature, which is added to the AssimpContruct to 
/// be exported afterward.
/// <summary>
class OSMDataImport : public osmium::handler::Handler {

public: 
  OSMDataImport(AssimpConstruct& ac, const ViewFilterList& filters );

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
  const ViewFilterList& mFilters;
  std::map<std::string, glm::vec3>  mMatColors;
  std::vector<OSMFeature> mFeatures;
  AssimpConstruct& mAssimpConstruct;
  aiNode* mParentNode;
};

}