
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


namespace GeoUtils {

/// <summary>
/// This class reads each OSM way node in a file and builds a map of S2 cell Ids, which cover each way.
/// A way may fall into more than one S2 Cell. Each Way in any S2 Cell will copy all it's nodes into that cell.
/// </summary>
class S2Splitter : public osmium::handler::Handler {

public:
  
  S2Splitter(int s2level);

  virtual ~S2Splitter();

  void setOutputDirectory(const std::string&);

  /// <summary>
  /// Set output format to xml, otherwise OSM's protobuf binary format is used.
  /// </summary>
  void setOutputXml(bool xml);


  /// <summary>
  /// If any "keys of interest" are added only ways containing those keys will be imported.
  /// </summary>
  void setKeyOfInterest(std::string key);

  
  void way(osmium::Way& way);
  void flush();

private:

  using SetOfNodeIds = std::unordered_set<osmium::unsigned_object_id_type>;
  
  std::unique_ptr<osmium::io::Writer>   getWriterForS2Cell(uint64_t cellId, osmium::io::Header& header);
  std::string                           fileNameOfS2Cell(uint64_t cellId);

  /// <summary>
  /// This struct keeps the buffer of OSM data for a given S2 cell.
  /// As more ways are added the SetOfNodeIds keeps track of individual added nodes to ensure duplicates are not added 
  /// <summary>
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

  std::vector<std::string> mKeysOfInterest;

};

}