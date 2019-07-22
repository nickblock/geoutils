#include "s2splitter.h"

#include "s2/s2cell.h"
#include "s2/s2latlng.h"
#include "s2/s2cell_id.h"
#include <osmium/io/input_iterator.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/builder/attr.hpp>

#include <sstream>

S2Splitter::S2Splitter(int s2Level)
: mS2Level(s2Level) 
{

}

S2Splitter::~S2Splitter() 
{

}

void S2Splitter::setOutputDirectory(const std::string& dir)
{
  mOutputDirectory = dir;
}

void S2Splitter::setOutputXml(bool xml)
{
  mOutXml = xml;
}
void S2Splitter::flush() 
{
  for(auto& iter : mS2CellDetails) {

    osmium::io::Header header;
    header.set("generator", "osms2splitter");
    header.add_box(iter.second.mBox);
    
    auto writer = getWriterForS2Cell(iter.first, header);
    (*writer)(std::move(iter.second.mBuffer));
  }
}

S2Splitter::S2CellDetails& S2Splitter::getS2CellDetails(uint64_t cellId)
{
  const auto& iter = mS2CellDetails.find(cellId);
  
  if(iter != mS2CellDetails.end()) {
    return iter->second;      
  }
  else {
    mS2CellDetails[cellId] = {
      SetOfNodeIds(),
      osmium::memory::Buffer{16, osmium::memory::Buffer::auto_grow::yes},
      osmium::Box()
    };
    return mS2CellDetails[cellId];
  }
}

std::unique_ptr<osmium::io::Writer> S2Splitter::getWriterForS2Cell(uint64_t cellId, osmium::io::Header& header)
{
  return std::make_unique<osmium::io::Writer>(fileNameOfS2Cell(cellId), header, osmium::io::overwrite::allow);
}

std::string S2Splitter::fileNameOfS2Cell(uint64_t cellId)
{
  std::stringstream ss;

  if(mOutputDirectory.size() > 0) {
    ss << mOutputDirectory;
    if(mOutputDirectory.back() != '/') {
      ss << '/';
    }
  }

  ss << "s2_" << std::hex << cellId << (mOutXml ? ".osm" : ".osm.pbf");
  return ss.str();
}
void S2Splitter::way(osmium::Way& way)
{
  auto& nodeList = way.nodes();

  std::unordered_set<uint64_t> cellsCovered;

  //create a list of s2cells which the nodes in this way occupy
  for(auto& node : nodeList) {

    cellsCovered.insert(
      S2Cell(S2LatLng::FromDegrees( // s2cell based on lat lon coords of node
        node.location().lat(), //node's lat lon coords
        node.location().lon()
      )) 
      .id().parent(mS2Level).id() // s2cell's id of parent at given level
    );
  }

  //for each s2cell covered by the way, add any nodes not yet added to it's file and then the way as well
  for(auto cellId : cellsCovered) {

    S2CellDetails& s2CellDetails = getS2CellDetails(cellId);

    for(auto& node : nodeList) {
      if(s2CellDetails.mWrittenNodes.find(node.ref()) == s2CellDetails.mWrittenNodes.end()) { // if the node is not already in the set we want to write it to the s2's file

        osmium::builder::add_node(s2CellDetails.mBuffer,
          osmium::builder::attr::_id(node.ref()),
          osmium::builder::attr::_location(node.location())
        );
        
        s2CellDetails.mBox.extend(node.location());

        s2CellDetails.mWrittenNodes.insert(node.ref());
      }
    }

    osmium::builder::add_way(s2CellDetails.mBuffer,
      osmium::builder::attr::_id(way.id()),
      osmium::builder::attr::_tags(way.tags()),
      osmium::builder::attr::_nodes(way.nodes())
    );
  }
}

