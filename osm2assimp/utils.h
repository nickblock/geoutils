
#pragma once

#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <osmium/osm/box.hpp>

using std::string;
using std::vector;

namespace GeoUtils
{
  struct BBox
  {
    BBox();
    void add(const glm::vec3 &p);
    void add(const BBox & /*bb*/);

    auto transform(const glm::mat4 & /*mat*/) -> BBox;

    glm::vec3 mMin{};
    glm::vec3 mMax{};

    glm::vec3 size();

    glm::vec3 fraction(const glm::vec3 &in);

    bool overlaps(const BBox &other) const;
  };

  osmium::Box osmiumBoxFromString(string extentsStr);
  osmium::Location refPointFromArg(string refPointStr);
  vector<string> getInputFiles(string input);
  std::string getFileExt(const std::string filename);

  class OSMDataImport;
  //special case: if the filename correlates to an S2 cell we use that as the relative center point of the file,
  // create a locator as the center of the s2 cell - this requires the global ref point to be set
  // In this way a number of S2Cells can be combined in the output file, with the geometry of each given
  // it's own ref point and parented to a unique parent node.
  void parentNodesToS2Cell(uint64_t s2cellId, OSMDataImport &importer);

  std::vector<glm::vec2> cornersFromBox(const osmium::Box &box);

  std::vector<std::vector<glm::vec2>> intersectPolygons(const std::vector<glm::vec2> &first, const std::vector<glm::vec2> &second);

  bool polyOrientation(const std::vector<glm::vec2> &poly);

  std::vector<glm::vec2> cleanPolyon(const std::vector<glm::vec2> &);

  BBox bBoxFromPoints2D(const std::vector<glm::vec2> &points);

} // namespace GeoUtils