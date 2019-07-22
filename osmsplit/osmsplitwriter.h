#ifndef OSMSPLIT_WRITER
#define OSMSPLIT_WRITER

#include <map>
#include <mutex>

#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/io/reader.hpp>
#include <osmium/osm/way.hpp>

#include "main.h"
#include "osmsplitconfig.h"

namespace EngineBlock {


class OSMSplitWriter {

  struct LockWriter {

    std::shared_ptr<osmium::io::Writer>  mWriter;
    std::shared_ptr<osmium::io::Writer>  mWayWriter;
    std::shared_ptr<std::mutex>          mMutex;
    std::string                          mOutWayPath;
    
    LockWriter() {}
    LockWriter(std::string outPath, osmium::io::Header& header);
    void write(osmium::memory::Buffer& buffer, const osmium::Way& way);
    void putWays();
  };

  void write(const std::string&     outFile,
            osmium::memory::Buffer& buffer, 
            const osmium::Way&      way
    );

public:
  OSMSplitWriter(
    OSMSplitConfigPtr rootConfig,
    std::string       inputFile,
    std::string       outputDirectory,
    NodeLocatorMap&   locStore,
    int               threads,
    uint64_t          wayCount
  );

  void writeWays(uint64_t start, uint64_t num);

protected:

  std::map<std::string, LockWriter>  mWriterMap;
  std::string                        mInputFileName;
  OSMSplitConfigPtr                  mRootConfig;
  OpCounter                          mOpCount;
  NodeLocatorMap&                    mNodeLocatorStore;
};

} // EngineBlock
#endif