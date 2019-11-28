#include "osmsplitwriter.h"

#include <osmium/io/input_iterator.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/builder/attr.hpp>

#include <experimental/filesystem>
#include <list>

using std::string;
using std::cout;
using std::endl;

namespace fs = std::experimental::filesystem;

namespace GeoUtils {

OSMSplitWriter::LockWriter::LockWriter(std::string outFilePath, osmium::io::Header& header) {

  int lastSlash = outFilePath.find(".osm.pbf");

  mOutWayPath = outFilePath.substr(0, lastSlash);
  mOutWayPath += "_ways.osm.pbf";

  mWayWriter = std::make_shared<osmium::io::Writer>(osmium::io::File(mOutWayPath), header, osmium::io::overwrite::allow);
  mWriter = std::make_shared<osmium::io::Writer>(osmium::io::File(outFilePath), header, osmium::io::overwrite::allow);

  mMutex = std::shared_ptr<std::mutex>(new std::mutex());
}
void OSMSplitWriter::LockWriter::write(osmium::memory::Buffer& buffer, const osmium::Way& way) {
  
  osmium::memory::Buffer bufferCopy{buffer.data(), buffer.committed()};

  std::lock_guard<std::mutex> g(*mMutex);

  (*mWriter)(std::move(bufferCopy));
  (*mWayWriter)(way);
  (*mWriter).flush();
}

void OSMSplitWriter::LockWriter::putWays() {
  
  mWayWriter = nullptr;

  osmium::io::File f{mOutWayPath};
  osmium::io::Reader reader{f, osmium::osm_entity_bits::way};
  auto ways = osmium::io::make_input_iterator_range<const osmium::Way>(reader);

  for(auto& way : ways) {
    (*mWriter)(way);
  }

  mWriter = nullptr;

  fs::remove(mOutWayPath);
}

OSMSplitWriter::OSMSplitWriter(
    OSMSplitConfigPtr rootConfig,
    std::string inputFile,
    std::string outputDirectory,
    NodeLocatorMap& locStore,
    int numThreads, 
    uint64_t wayCount
  )

: mInputFileName(inputFile),
  mRootConfig(rootConfig),
  mNodeLocatorStore(locStore)

{
  int outCount = 0;

  auto configList = rootConfig->getLeafNodes();

  for(auto& config : configList) {
    
    string outFileName = config->getFileName();
    string outFilePath = outputDirectory + outFileName;

    osmium::io::Header header;
    header.set("generator", "osmsplit");
    header.add_box(config->getBox());

    mWriterMap[outFileName] = LockWriter(outFilePath, header);
  }

  mOpCount.setOps(wayCount + mWriterMap.size());

  uint64_t countPerThread = wayCount / numThreads;
  uint64_t start = 0;

  std::list<std::thread> threads;
  for(int i=0; i<numThreads; i++) {

    uint64_t len = countPerThread;
    if(wayCount < start + len) {
      len = wayCount - start;
    }

    threads.push_back(std::thread(
                    &OSMSplitWriter::writeWays, this, start, len
                  ));
    start += len;
  }

  for(auto& t : threads) {
    t.join();
  }

  mNodeLocatorStore.clear();

  cout << "Consolidate files: " << mWriterMap.size() << endl; 

  mOpCount.setOps(mWriterMap.size());

  threads.clear();
  for(auto& w : mWriterMap) {

    threads.push_back(std::thread(
      &OSMSplitWriter::LockWriter::putWays, &w.second)
    );

    if(threads.size() == numThreads) {
      for(auto& t : threads) {
        t.join();
        mOpCount.countOff(1);
      }
      threads.clear();
    }
  }
  for(auto& t : threads) {
    t.join();
    mOpCount.countOff(1);
  }
}

using namespace osmium::builder::attr;
const size_t initial_buffer_size = 16;

void OSMSplitWriter::writeWays(uint64_t start, uint64_t num)
{
  osmium::io::File f{mInputFileName};
  osmium::io::Reader reader{f, osmium::osm_entity_bits::way};
  auto ways = osmium::io::make_input_iterator_range<const osmium::Way>(reader);

  uint64_t count = 0;
  int opCount = 0;
  for(auto& way : ways) {

    count++;
    if(count-1 < start) continue;
    if(count-1 >= start+num) break; 

    osmium::Box boxForWay;

    osmium::memory::Buffer wayBuffer{initial_buffer_size, osmium::memory::Buffer::auto_grow::yes};

    for (const auto& node : way.nodes()) {

      const osmium::Location& loc = mNodeLocatorStore.get(node.ref());

      boxForWay.extend(loc);
      
      osmium::builder::add_node(wayBuffer,
        _id(node.ref()),
        _location(loc)
      );
    }

    wayBuffer.commit();

    auto fileList = mRootConfig->filesForBox(boxForWay);

    for(auto& file : fileList) {
      mWriterMap[file].write(wayBuffer, way);
    }

    if(opCount++ > mOpCount.totalOps() / 100) {
      mOpCount.countOff(opCount);
      opCount = 0;
    }
  }
}


} // EngineBlock