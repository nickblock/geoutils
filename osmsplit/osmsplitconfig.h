#ifndef OSMSPLIT_H
#define OSMSPLIT_H

#include "rapidjson/document.h"

#include <osmium/osm/box.hpp>
#include <osmium/osm/location.hpp>

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <filesystem>

namespace fs = std::filesystem;

namespace GeoUtils {

class OSMSplitConfig;

using OSMSplitConfigPtr = std::shared_ptr<OSMSplitConfig>;
using OSMConfigList = std::vector<OSMSplitConfigPtr>;

class OSMSplitConfig {

  using OSMSplitConfigPair = std::pair<OSMSplitConfigPtr, OSMSplitConfigPtr>;

public:
  OSMSplitConfig() {};
  OSMSplitConfig(const osmium::Box &ext, const fs::path &outputPrefix,
                 bool sortByLat = true);
  OSMSplitConfig(const rapidjson::Value &);

  void initFromJSON(const rapidjson::Value &);
  OSMSplitConfigPair split(double midPoint);
  void setFileName(const fs::path &fileName);
  static void setOutputSuffix(std::string s);

  rapidjson::Value toJson(rapidjson::Document::AllocatorType &alloc) const;

  std::vector<fs::path> filesForBox(const osmium::Box &box) const;
  OSMConfigList getLeafNodes() const;
  fs::path getFileName() const;

  const osmium::Box &getBox() const { return mExtents; }
  bool sortByLat() const { return mSortByLat; }
  bool isLeaf() const { return mSplit.first == nullptr; }
  static fs::path suffix() { return mSuffix; }

protected:
  osmium::Box mExtents;
  bool mSortByLat;
  double mMidPoint;
  OSMSplitConfigPair mSplit;
  fs::path mFileName;
  static fs::path mSuffix;
};

} // namespace GeoUtils

#endif