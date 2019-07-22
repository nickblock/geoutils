#ifndef OSM_DATA_H
#define OSM_DATA_H

#include <string>
#include <map>
#include <vector>
#include "common_geo.h"
#include "osmfeature.h"
#include <osmium/handler.hpp>

class AssimpConstruct;

namespace xmlpp {
  class Node;
}

class OSMDataImport : public osmium::handler::Handler {
public: 
  OSMDataImport(AssimpConstruct& ac, const osmium::Box& extents, int filter = 0xffffffff );

  void way(const osmium::Way& way);

  void node(const osmium::Node& node);

  int exportCount() {
    return mCount;
  }
protected:

  void process(const OSMFeature& feature);

  int mCount;
  int mFilter;
  const osmium::Box& mExtents;
  WorldCoord mRefPoint;
  std::map<std::string, glm::vec3>  mMatColors;
  std::vector<OSMFeature> mFeatures;
  AssimpConstruct& mAssimpConstruct;
};
#endif