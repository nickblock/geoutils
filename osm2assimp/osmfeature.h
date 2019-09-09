#include <vector>
#include <string>
#include "common_geo.h"

struct OSMFeature {

  static const int UNDEFINED = 0;
  static const int CLOSED = 1;
  static const int BUILDING = 2;
  static const int WATER = 4;
  static const int HIGHWAY = 8;
  static const int LOCATION = 16;

  OSMFeature(const osmium::Way& way, const WorldCoord& ref, bool findName = false);
  OSMFeature(const osmium::Node& node, const WorldCoord& ref, bool findName = false);

  int Type() const { return mType; }

  int mType;
  float mHeight;
  int mFloors;
  std::string mMaterial;
  std::string mName;
  std::vector<glm::vec2> worldCoords;
  bool mValid;
};
  