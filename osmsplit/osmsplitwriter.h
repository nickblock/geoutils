#ifndef OSMSPLIT_WRITER
#define OSMSPLIT_WRITER

#include <filesystem>
#include <map>
#include <mutex>

#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/reader.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/osm/way.hpp>

#include "main.h"
#include "osmsplitconfig.h"

namespace GeoUtils {

class OSMSplitWriter {

  struct LockWriter {

    std::shared_ptr<osmium::io::Writer> mWriter;
    std::shared_ptr<osmium::io::Writer> mWayWriter;
    std::shared_ptr<std::mutex> mMutex;
    fs::path mOutWayPath;

    LockWriter() {}
    LockWriter(fs::path outPath, osmium::io::Header &header);
    void write(osmium::memory::Buffer &buffer, const osmium::Way &way);
    void putWays();
  };

  void write(const fs::path &outFile, osmium::memory::Buffer &buffer,
             const osmium::Way &way);

public:
  OSMSplitWriter(OSMSplitConfigPtr rootConfig, fs::path inputFile,
                 fs::path outputDirectory, NodeLocatorMap &locStore,
                 int threads, uint64_t wayCount);

  void writeWays(uint64_t start, uint64_t num);

protected:
  std::map<fs::path, LockWriter> mWriterMap;
  fs::path mInputFileName;
  OSMSplitConfigPtr mRootConfig;
  OpCounter mOpCount;
  NodeLocatorMap &mNodeLocatorStore;
};

} // namespace GeoUtils
#endif