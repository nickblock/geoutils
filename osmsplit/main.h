#ifndef MAIN_OSMSPLIT_H
#define MAIN_OSMSPLIT_H

#include "osmsplitconfig.h"
#define OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_ARRAY
#include <osmium/index/id_set.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/osm/box.hpp>

#include "tinyformat.h"

#include <iostream>
#include <string>

#include <filesystem>

namespace fs = std::filesystem;

using NodeLocatorMap =
    osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type,
                                       osmium::Location>;

std::string constructOutDirName(const std::string &inputFileArg,
                                const std::string &outputDirArg);
std::string fileNameFromPath(const std::string &path);
int getMemUse();
int getElapsedTime();
void printMemTimeUpdate();

class OpCounter {

public:
  void setOps(int ops) { mTotal = mOps = ops; }

  float percentage() { return (float)mOps * 100.f / mTotal; }

  void countOff(int c) {
    std::lock_guard<std::mutex> g(mMutex);

    mOps -= c;

    std::cout << tfm::format("Remaining [%2.2f%%] Mem : %d, Time : %d",
                             percentage(), getMemUse(), getElapsedTime())
              << std::endl;
  }
  int totalOps() { return mTotal; }

protected:
  int mTotal;
  int mOps;
  std::mutex mMutex;
};

struct SplitOptions {
  int depthLevels = 1;
  int sampleRate = 1;
  int threadNum = 1;
  bool updateOnly = false;
  bool deleteInputFiles = false;
};

void processOSMFile(const fs::path &inputFile, const fs::path &outDir,
                    GeoUtils::OSMSplitConfigPtr &config, SplitOptions options);

void processConfigFile(const fs::path &inputFile, const fs::path &outDir,
                       GeoUtils::OSMSplitConfigPtr &config,
                       SplitOptions options);

#endif