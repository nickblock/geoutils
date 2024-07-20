#include "osmsplitconfig.h"

#include <iostream>

using std::cout;
using std::endl;
using std::make_shared;
using std::string;

namespace GeoUtils {

void printBox(const osmium::Box &box) {
  cout << "min " << box.bottom_left().lat() << "," << box.bottom_left().lon()
       << " max " << box.top_right().lat() << ", " << box.top_right().lon();
}

rapidjson::Value osmiumBoxToJSON(const osmium::Box &box,
                                 rapidjson::Document::AllocatorType &alloc) {

  rapidjson::Value value(rapidjson::kObjectType);
  rapidjson::Value minLoc(rapidjson::kArrayType);
  rapidjson::Value maxLoc(rapidjson::kArrayType);

  minLoc.PushBack(box.bottom_left().lon(), alloc);
  minLoc.PushBack(box.bottom_left().lat(), alloc);

  maxLoc.PushBack(box.top_right().lon(), alloc);
  maxLoc.PushBack(box.top_right().lat(), alloc);

  value.AddMember("min", minLoc, alloc);
  value.AddMember("max", maxLoc, alloc);

  return value;
}

osmium::Box osmiumBoxFromJSON(const rapidjson::Value &v) {

  osmium::Box box;

  const auto &minJS = v["min"];
  const auto &maxJS = v["max"];

  box.extend(osmium::Location(minJS[0].GetDouble(), minJS[1].GetDouble()));
  box.extend(osmium::Location(maxJS[0].GetDouble(), maxJS[1].GetDouble()));

  return box;
}

fs::path OSMSplitConfig::mSuffix = ".osm.pbf";

OSMSplitConfig::OSMSplitConfig(const osmium::Box &ext,
                               const fs::path &outputPrefix, bool sortByLat)
    : mExtents(ext), mSortByLat(sortByLat), mFileName(outputPrefix) {}

OSMSplitConfig::OSMSplitConfig(const rapidjson::Value &value)

{
  initFromJSON(value);
}

void OSMSplitConfig::setOutputSuffix(std::string s) { mSuffix = s; }

void OSMSplitConfig::initFromJSON(const rapidjson::Value &value) {
  mExtents = osmiumBoxFromJSON(value["extents"]);
  if (value.HasMember("splitLess")) {

    mMidPoint = value["midPoint"].GetDouble();
    mSortByLat = value["sortByLat"].GetBool();

    mSplit.first = make_shared<OSMSplitConfig>(value["splitLess"]);
    mSplit.second = make_shared<OSMSplitConfig>(value["splitMore"]);
  } else {
    if (abs(mExtents.top_right().lat() - mExtents.bottom_left().lat()) >
        abs(mExtents.top_right().lat() - mExtents.bottom_left().lat())) {
      mSortByLat = true;
    } else {
      mSortByLat = false;
    }
    mFileName = value["fileName"].GetString();
  }
}
OSMSplitConfig::OSMSplitConfigPair OSMSplitConfig::split(double midPoint) {
  mMidPoint = midPoint;

  osmium::Box lessBox, moreBox;

  lessBox.extend(mExtents.bottom_left());
  moreBox.extend(mExtents.top_right());

  if (mSortByLat) {

    lessBox.extend(osmium::Location(mExtents.top_right().lon(), midPoint));
    moreBox.extend(osmium::Location(mExtents.bottom_left().lon(), midPoint));
  } else {

    lessBox.extend(osmium::Location(midPoint, mExtents.top_right().lat()));
    moreBox.extend(osmium::Location(midPoint, mExtents.bottom_left().lat()));
  }

  mSplit.first = make_shared<OSMSplitConfig>(
      lessBox, mFileName.string() + string("0"), !mSortByLat);
  mSplit.second = make_shared<OSMSplitConfig>(
      moreBox, mFileName.string() + string("1"), !mSortByLat);

  return mSplit;
}

rapidjson::Value
OSMSplitConfig::toJson(rapidjson::Document::AllocatorType &allocJS) const {
  rapidjson::Value configJS(rapidjson::kObjectType);

  configJS.AddMember("extents", osmiumBoxToJSON(mExtents, allocJS), allocJS);

  if (mSplit.first) {

    configJS.AddMember("midPoint", mMidPoint, allocJS);
    configJS.AddMember("sortByLat", mSortByLat, allocJS);
    configJS.AddMember("splitLess", mSplit.first->toJson(allocJS), allocJS);
    configJS.AddMember("splitMore", mSplit.second->toJson(allocJS), allocJS);
  } else {

    configJS.AddMember("fileName", mFileName.string(), allocJS);
  }

  return configJS;
}

std::vector<fs::path>
OSMSplitConfig::filesForBox(const osmium::Box &box) const {
  std::vector<fs::path> fileNames;

  if (box.bottom_left().x() > mExtents.top_right().x() ||
      box.bottom_left().y() > mExtents.top_right().y() ||
      box.top_right().x() < mExtents.bottom_left().x() ||
      box.top_right().y() < mExtents.bottom_left().y()) {
    return fileNames;
  } else {
    if (mSplit.first) {
      std::vector<fs::path> firstNames = mSplit.first->filesForBox(box);
      if (firstNames.size()) {
        fileNames.insert(fileNames.end(), firstNames.begin(), firstNames.end());
      }
      std::vector<fs::path> secondNames = mSplit.second->filesForBox(box);
      if (secondNames.size()) {
        fileNames.insert(fileNames.end(), secondNames.begin(),
                         secondNames.end());
      }
    } else {
      fileNames.push_back(getFileName());
    }
  }

  return fileNames;
}

OSMConfigList OSMSplitConfig::getLeafNodes() const {
  OSMConfigList list;
  if (mSplit.first) {

    if (mSplit.first->isLeaf()) {
      list.push_back(mSplit.first);
      list.push_back(mSplit.second);
    } else {

      auto fl = mSplit.first->getLeafNodes();
      list.insert(list.end(), fl.begin(), fl.end());

      auto sl = mSplit.second->getLeafNodes();
      list.insert(list.end(), sl.begin(), sl.end());
    }
  }
  return list;
}
fs::path OSMSplitConfig::getFileName() const {
  return mFileName.string() + suffix().string();
}
} // namespace GeoUtils