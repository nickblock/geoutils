
#pragma once

#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/io/writer.hpp>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <memory>
#include <stdint.h>


class S2Splitter : public osmium::handler::Handler {

public:
  
  S2Splitter(int s2level);

  virtual ~S2Splitter();

  void setOutputDirectory(const std::string&);
  void setOutputXml(bool xml);

  void way(osmium::Way& way);

  void flush();

private:

  using SetOfNodeIds = std::unordered_set<osmium::unsigned_object_id_type>;
  
  std::unique_ptr<osmium::io::Writer>   getWriterForS2Cell(uint64_t cellId, osmium::io::Header& header);
  std::string                           fileNameOfS2Cell(uint64_t cellId);

  struct S2CellDetails {
    SetOfNodeIds mWrittenNodes;
    osmium::memory::Buffer mBuffer;
    osmium::Box mBox;
  };

  std::unordered_map<uint64_t, S2CellDetails> mS2CellDetails;

  S2CellDetails& getS2CellDetails(uint64_t cellId);

  int mS2Level = 10;
  std::string mOutputDirectory;
  bool mOutXml = false;

};