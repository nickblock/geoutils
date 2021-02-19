#include "osmdata.h"
#include "sceneconstruct.h"
#include "geomconvert.h"
#include <iostream>
#include <algorithm>

#include <osmium/handler/node_locations_for_ways.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/area/geom_assembler.hpp>

#include "convertlatlng.h"

#include "common.h"

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;

namespace GeoUtils
{

  OSMDataImport::OSMDataImport(SceneConstruct &ac, const ViewFilterList &filters)
      : mCount(0),
        mSceneConstruct(ac),
        mFilters(filters),
        mParentNode(nullptr)
  {
    std::locale::global(std::locale(""));

    mMatColors["ground"] = glm::vec3(149 / 255.f, 174 / 255.f, 81 / 255.f);
    mMatColors["highway"] = glm::vec3(81 / 255.f, 149 / 255.f, 174 / 255.f);
    mMatColors["water"] = glm::vec3(0.2, 0.4, 0.8);
    mMatColors["green"] = glm::vec3(0.2, 0.7, 0.1);
    mMatColors["red"] = glm::vec3(0.8, 0.1, 0.2);
    mMatColors["default"] = glm::vec3(174 / 255.f, 81 / 255.f, 149 / 255.f);
    mMatColors["building"] = glm::vec3(0.9, 0.1, 0.7);
  }

  void OSMDataImport::way(const osmium::Way &way)
  {
    for (const auto &filter : mFilters)
    {

      if (!filter->include(way))
      {
        return;
      }
    }

    OSMFeature feature(way);

    if (feature.isValid())
    {
    }
  }

  void OSMDataImport::node(const osmium::Node &node)
  {
  }

  void OSMDataImport::setParentAINode(aiNode *node)
  {
    // mParentNode = node;

    // mSceneConstruct.addLocator(node);
  }

} // namespace GeoUtils