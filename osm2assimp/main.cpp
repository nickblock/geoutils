#include "../args.hxx"
#include "../tinyformat.h"
#include "assimp_construct.h"
#include "common_geo.h"
#include "osmdata.h"
#include "centerearthfixedconvert.h"

#include <string.h>
#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <ctime>

#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/geom/mercator_projection.hpp>
using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

using namespace std;

const float buildingFloorHeight = 3.5f;

osmium::Box osmiumBox(string extentsStr)
{
  osmium::Box box;

  osmium::Location locMin, locMax;

  int commaPos = extentsStr.find_first_of(",");
  locMin.set_lon(std::stod(extentsStr.substr(0, commaPos)));
  extentsStr = extentsStr.substr(commaPos+1, extentsStr.size());

  commaPos = extentsStr.find_first_of(",");
  locMin.set_lat(std::stod(extentsStr.substr(0, commaPos)));
  extentsStr = extentsStr.substr(commaPos+1, extentsStr.size());

  commaPos = extentsStr.find_first_of(",");
  locMax.set_lon(std::stod(extentsStr.substr(0, commaPos)));
  extentsStr = extentsStr.substr(commaPos+1, extentsStr.size());

  locMax.set_lat(std::stod(extentsStr));

  box.extend(locMin);
  box.extend(locMax);

  return box;
}

osmium::Location refPointFromArg(string refPointStr) 
{
 osmium::Location location;

  int commaPos = refPointStr.find_first_of(",");
  location.set_lon(std::stod(refPointStr.substr(0, commaPos)));
  refPointStr = refPointStr.substr(commaPos+1, refPointStr.size());
  location.set_lat(std::stod(refPointStr));

 return location;
}

int main(int argi, char** argc)
{

  cout << "Running osm2assimp " << endl;

  std::clock_t start = std::clock();

  AssimpConstruct assimpConstruct;

  args::ArgumentParser parser("osm2assimp, convert osm files to geometry to be saved in a assimp compatible format.", "You must at least specify an input and an output file");
  args::HelpFlag help(parser, "HELP", "Show this help menu.", {'h', "help"});
  args::ValueFlag<std::string> inputFileArg(parser, "*.osm|*.pbf", "Specify input .osm file", {'i'});
  args::ValueFlag<std::string> outputFileArg(parser, assimpConstruct.formatsAvailableStr(), "Specify output file. The extension will be used to decide the output type. The type should be compatible with assimp", {'o'});
  args::ValueFlag<float> fixedHeightArg(parser, "100.0", "Specify a default height to be used in absence of heights (.dbf) file", {'f'});
  args::ValueFlag<int> limitArg(parser, "0", "Specify an upper limit fo shapes to import", {'l'});
  args::Flag  useMax(parser, "Max", "Use max instead of default mean value for heights", {'m'});
  args::Flag  consolidateArg(parser, "Consolidate", "Consolidate mesh data into single mesh", {'c'});
  args::Flag  highwayArg(parser, "Highways", "Include roads in export", {'r'});
  args::ValueFlag<string> extentsArg(parser, "Extents", "4 comma separated values; min lat, min long, max lat, max long", {'e'});
  args::ValueFlag<string> refPointArg(parser, "RefPoint", "2 comma separated values; latLng coords. To be used as point of origin for geometry ", {'e'});

  try
  {
      parser.ParseCLI(argi, argc);
  }
  catch (args::Help)
  {
      std::cout << parser;
      return 0;
  }
  catch (args::ParseError e)
  {
      std::cerr << e.what() << std::endl;
      std::cerr << parser;
      return 1;
  }
  catch (args::ValidationError e)
  {
      std::cerr << e.what() << std::endl;
      std::cerr << parser;
      return 1;
  }
  if(!inputFileArg || !outputFileArg) {

      cout << parser;
      return 0;
  }

  if(consolidateArg) {
    assimpConstruct.setConsolidateMesh(true);
  }

  string outputFile = args::get(outputFileArg);
  string outExt = outputFile.substr(outputFile.size()-3, 3);

  
  if(!assimpConstruct.checkFormat(outExt)) {
    cout << "Couldnt find filetype for extension '" << outExt << "', the avialable types are " << assimpConstruct.formatsAvailableStr()  << endl;
    return 1;
  }

  int shapeLimit = limitArg ? args::get(limitArg) : 0;

  string inputFile = args::get(inputFileArg);
  string inputExt = inputFile.substr(inputFile.size() -3, 3);

  osmium::Box box;

  if(extentsArg) {

    try {

      string extentsStr = args::get(extentsArg);

      box = osmiumBox(extentsStr);
    }
    catch(std::invalid_argument) {
      cout << "Failed to parse extents string '" << args::get(extentsArg) << "'" << endl;
      std::exit(1);
    }
  }

  osmium::Location refPoint;

  if(refPointArg) {
    try {

      refPoint = refPointFromArg(args::get(refPointArg));
    }
    catch(std::invalid_argument) {
      cout << "failed to parse ref point string '" << args::get(refPointArg) << "'" << endl;
      std::exit(1);
    }
  }

  if(inputFile == "test") {

    vector<glm::vec2> verts(4);
    verts[0]= glm::vec2(-10, -10);
    verts[1]= glm::vec2(-10, 10);
    verts[2]= glm::vec2(10, 10);
    verts[3]= glm::vec2(10, -10);

    assimpConstruct.addMesh(assimpConstruct.extrude2dMesh(verts, 20.f), "testmesh");

    cout << "added test mesh" << endl;
  }


  if(inputExt == "osm" || inputExt == "pbf") {

    index_type index;

    location_handler_type location_handler{index};
    location_handler.ignore_errors();
    
    osmium::io::Reader reader{inputFile, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way};

    if(!box.valid()) {
      osmium::io::Header header = reader.header();
      box = header.box();
    }

    int filter = OSMFeature::BUILDING;
    if(highwayArg) {
      filter |= OSMFeature::HIGHWAY;
    }

    cout << "Extents : " << tfm::format("min(%2.2f, %2.2f) - max(%2.2f, %2.2f)", 
      box.bottom_left().lon(), box.bottom_left().lat(), box.top_right().lon(), box.top_right().lat()) << endl;

    EngineBlock::CenterEarthFixedConvert::refPoint = box.bottom_left();

    if(refPoint.valid()) {
      EngineBlock::CenterEarthFixedConvert::refPoint = refPoint;
    }

    OSMDataImport osmReader(assimpConstruct, box, filter);

    osmium::apply(reader, location_handler, osmReader);

    cout << "Ways Exported: " << osmReader.exportCount() << endl;
  }

  if(AI_SUCCESS != assimpConstruct.write(outputFile.c_str())){
    cout << "Failed to write out to '" << outputFile << "'" << endl;
    return 1;
  }

  double elapsed = (std::clock() - start)/(double)CLOCKS_PER_SEC; 

  cout << "Time Taken : " << elapsed << endl;

  return 0;
}
