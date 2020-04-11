#include "../args.hxx"
#include "../tinyformat.h"
#include "assimpconstruct.h"
#include "geomconvert.h"
#include "osmdata.h"
#include "convertlatlng.h"
#include "eigenconversion.h"
#include "s2util.h"
#include "viewfilter.h"

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
#include <osmium/geom/coordinates.hpp>
#include <osmium/osm/way.hpp>

using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

using std::vector;
using std::cout;
using std::endl;
using std::make_shared;

using GeoUtils::ConvertLatLngToCoords;
using GeoUtils::OSMDataImport;
using GeoUtils::AssimpConstruct;
using GeoUtils::GeomConvert;
using GeoUtils::OSMFeature;
using GeoUtils::S2Util;
using GeoUtils::ViewFilterList;
using GeoUtils::TypeFilter;
using GeoUtils::BoundFilter;
using GeoUtils::S2CellFilter;

osmium::Box osmiumBoxFromString(string extentsStr)
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

vector<string> getInputFiles(string input)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(input);
  while (std::getline(tokenStream, token, ',')) {
    if(token.size()) {
      tokens.push_back(token);
    }
  }
  return tokens;
}

osmium::Location refPointFromArg(string refPointStr) 
{
  osmium::Location location;

  int commaPos = refPointStr.find_first_of(",");
  location.set_lat(std::stod(refPointStr));
  refPointStr = refPointStr.substr(commaPos+1, refPointStr.size());
  location.set_lon(std::stod(refPointStr.substr(0, commaPos)));

 return location;
}

//special case: if the filename correlates to an S2 cell we use that as the relative center point of the file,
// create a locator as the center of the s2 cell - this requires the global ref point to be set
// In this way a number of S2Cells can be combined in the output file, with the geometry of each given
// it's own ref point and parented to a unique parent node. 
void parentNodesToS2Cell(uint64_t s2cellId, OSMDataImport& importer)
{
  S2Util::LatLng s2LatLng = S2Util::getS2Center(s2cellId);    
  osmium::Location s2CellCenterLocation = osmium::Location(std::get<1>(s2LatLng), std::get<0>(s2LatLng));

  //the coords are given relative to the originLocation
  const osmium::geom::Coordinates s2CellCenterCoord = ConvertLatLngToCoords::to_coords(s2CellCenterLocation);
  ConvertLatLngToCoords::setRefPoint(s2CellCenterLocation);

  aiNode* s2CellParentAINode = new aiNode(std::to_string(s2cellId));

  aiVector3t<float> asm_pos(s2CellCenterCoord.x, 0.0f, s2CellCenterCoord.y);
  aiMatrix4x4t<float>::Translation(asm_pos, s2CellParentAINode->mTransformation);

  importer.setParentAINode(s2CellParentAINode);
}

int main(int argi, char** argc)
{

  cout << "Running osm2assimp " << endl;

  std::clock_t start = std::clock();

  AssimpConstruct assimpConstruct;

  args::ArgumentParser parser("osm2assimp, convert osm files to geometry to be saved in a assimp compatible format.", "You must at least specify an input and an output file");
  args::HelpFlag help(parser, "HELP", "Show this help menu.", {'h', "help"});
  args::ValueFlag<std::string> inputFileArg(parser, "*.osm|*.pbf", "Specify input .osm file or comma separated list of files.", {'i'});
  args::ValueFlag<std::string> outputFileArg(parser, assimpConstruct.formatsAvailableStr(), "Specify output file. The extension will be used to decide the output type. The type should be compatible with assimp", {'o'});
  args::ValueFlag<float> fixedHeightArg(parser, "100.0", "Specify a default height to be used in absence of heights data", {'f'});
  args::ValueFlag<int> limitArg(parser, "0", "Specify an upper limit fo shapes to import", {'l'});
  args::Flag  consolidateArg(parser, "Consolidate", "Consolidate mesh data into single mesh", {'c'});
  args::Flag  highwayArg(parser, "Highways", "Include roads in export", {'r'});
  args::Flag  exportZUpArg(parser, "Z Up", "Export Z up", {'z'});
  args::Flag  convertCEF(parser, "Convert CEF", "When converting LatLng to coordinate use Center Earth Fixed algorithm, as opposed to the default mercator projection", {'a'});
  args::ValueFlag<string> extentsArg(parser, "Extents", "4 comma separated values; min lat, min long, max lat, max long", {'e'});
  args::ValueFlag<string> refPointArg(parser, "RefPoint", "2 comma separated values; latLng coords. To be used as point of origin for geometry ", {'p'});
  args::ValueFlag<string> s2CellArg(parser, "S2Cell", "S2 Cell Id as hex string. Filters nodes and ways only somepart inside S2 Cell", {'s', "s2"});

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
  if(!inputFileArg || !outputFileArg) 
  {
      cout << parser;
      return 0;
  }

  if(consolidateArg) {
    assimpConstruct.setConsolidateMesh(true);
  }

  if(exportZUpArg) {
    GeomConvert::zUp = true;
  }

  if(convertCEF) {
    ConvertLatLngToCoords::UseCenterEarthFixed = true;
  }

  string outputFile = args::get(outputFileArg);
  string outExt = outputFile.substr(outputFile.size()-3, 3);

  
  if(!assimpConstruct.checkFormat(outExt)) {
    cout << "Couldnt find filetype for extension '" << outExt << "', the avialable types are " << assimpConstruct.formatsAvailableStr()  << endl;
    return 1;
  }

  ViewFilterList viewFilters;

  //the originLocation will decide the point of origin for the exported geometry file.
  //if it's not decided by the rePoint cmd line arg it is taken from the bottom left
  //corner of the box, which in turn can be decided by the extents argument, 
  // or is otherwise taken from the bounds of the input file.
  osmium::Location originLocation; 

  osmium::Box box;

  if(extentsArg) {

    try {

      string extentsStr = args::get(extentsArg);

      box = osmiumBoxFromString(extentsStr);

      originLocation = box.bottom_left();
      ConvertLatLngToCoords::setRefPoint(originLocation);

      viewFilters.push_back(make_shared<BoundFilter>(box));
    }
    catch(std::invalid_argument) {
      cout << "Failed to parse extents string '" << args::get(extentsArg) << "'" << endl;
      std::exit(1);
    }
  }

  if(refPointArg) {
    try {

      originLocation = refPointFromArg(args::get(refPointArg));
      ConvertLatLngToCoords::setRefPoint(originLocation);
    }
    catch(std::invalid_argument) {
      cout << "failed to parse ref point string '" << args::get(refPointArg) << "'" << endl;
      std::exit(1);
    }
  }

  if(s2CellArg) {
    try {

      string s2CellStr = args::get(s2CellArg);
      if(s2CellStr.size() < 16) {
        int addZeroes = 16 - s2CellStr.size();
        for(int i=0; i<addZeroes; i++) s2CellStr += "0";
      }

      uint64_t s2cellId = S2Util::getS2IdFromString(s2CellStr);

      viewFilters.push_back(make_shared<S2CellFilter>(s2cellId));

      S2Util::LatLng latLng = S2Util::getS2Center(s2cellId);
      originLocation = {std::get<1>(latLng), std::get<0>(latLng)};
      ConvertLatLngToCoords::setRefPoint(originLocation);
    }
    catch(const std::exception&) {

      cout << "failed to parse S2 Cell id string '" << args::get(s2CellArg) << "', is it a <=16 character hex string?" << endl;
      std::exit(1);
    }
  }

  vector<string> inputFiles = getInputFiles(args::get(inputFileArg));

  for(auto& inputFile : inputFiles) {

    cout << inputFile << endl;

    string inputExt = inputFile.substr(inputFile.size() -3, 3);

    if(inputExt == "osm" || inputExt == "pbf") {

      index_type index;

      location_handler_type locationHandler{index};

      try {

        osmium::io::Reader osmFileReader{inputFile, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way};

        if(!box.valid()) {
          osmium::io::Header header = osmFileReader.header();
          box = header.box();

          if(!box.valid() && !refPointArg) {
            cout << "No bounds found in osm file and no RefPoint specified, aborting" << endl;
            exit(1);
          }
        }

        //if the refPoint was not set via commandline take the bottom left of the first input file specified 
        if(!originLocation) {
          originLocation = box.bottom_left();
          ConvertLatLngToCoords::setRefPoint(originLocation);
        }

        int filter = OSMFeature::BUILDING;
        if(highwayArg) {
          filter |= OSMFeature::HIGHWAY;
        }

        viewFilters.push_back(make_shared<TypeFilter>(filter));

        OSMDataImport geoDataImporter(assimpConstruct, viewFilters);

        osmium::apply(osmFileReader, locationHandler, geoDataImporter);

        cout << "Ways Exported: " << geoDataImporter.exportCount() << endl;
      }
      catch(const std::system_error& err) {
        cout << "Failed to read " << inputFile << ", " << err.what() << endl;
        continue;
      }
    }
  }

  if(assimpConstruct.numMeshes()) {
    if(AI_SUCCESS != assimpConstruct.write(outputFile.c_str())){
      cout << "Failed to write out to '" << outputFile << "', " << assimpConstruct.exporterErrorStr() << endl;
      return 1;
    }
  }
  else {
    cout << "No geometry found, nothing to write" << endl;
  }

  double elapsed = (std::clock() - start)/(double)CLOCKS_PER_SEC; 

  cout << "Time Taken : " << elapsed << endl;

  return 0;
}
