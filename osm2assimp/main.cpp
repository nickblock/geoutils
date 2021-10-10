#include "../args.hxx"
#include "../tinyformat.h"
#include "sceneconstruct.h"
#include "assimpwriter.h"
#include "geomconvert.h"
#include "convertlatlng.h"
#include "eigenconversion.h"
#include "utils.h"
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

using std::cout;
using std::endl;
using std::make_shared;
using std::vector;

using GeoUtils::AssimpWriter;
using GeoUtils::BoundFilter;
using GeoUtils::ConvertLatLngToCoords;
using GeoUtils::cornersFromBox;
using GeoUtils::GeomConvert;
using GeoUtils::getFileExt;
using GeoUtils::getInputFiles;
using GeoUtils::OSMDataImport;
using GeoUtils::OSMFeature;
using GeoUtils::osmiumBoxFromString;
using GeoUtils::refPointFromArg;
using GeoUtils::S2CellFilter;
using GeoUtils::S2Util;
using GeoUtils::SceneConstruct;
using GeoUtils::TypeFilter;
using GeoUtils::ViewFilterList;

int main(int argi, char **argc)
{

  cout << "Running osm2assimp " << endl;

  std::clock_t start = std::clock();

  AssimpWriter assimpWriter;

  args::ArgumentParser parser("osm2assimp, convert osm files to geometry to be saved in a assimp compatible format.", "You must at least specify an input and an output file");
  args::HelpFlag help(parser, "HELP", "Show this help menu.", {'h', "help"});
  args::ValueFlag<std::string> inputFileArg(parser, "*.osm|*.pbf", "Specify input .osm file or comma separated list of files.", {'i'});
  args::ValueFlag<std::string> outputFileArg(parser, assimpWriter.formatsAvailableStr(), "Specify output file. The extension will be used to decide the output type. The type should be compatible with assimp", {'o'});
  args::ValueFlag<float> fixedHeightArg(parser, "100.0", "Specify a default height to be used in absence of heights data", {'f'});
  args::ValueFlag<int> consolidateArg(parser, "Consolidate", "Consolidate mesh data into single mesh '0' or Mesh per Object '2'. Default is mesh per Material/Type '1'", {'c'});
  args::Flag highwayArg(parser, "Highways", "Include roads in export", {'r'});
  args::Flag exportZUpArg(parser, "Z Up", "Export Z up", {'z'});
  args::Flag groundArg(parser, "Ground", "Create ground plane to extents or S2 cell region", {'g'});
  args::Flag convertCEF(parser, "Convert CEF", "When converting LatLng to coordinate use Center Earth Fixed algorithm, as opposed to the default mercator projection", {'a'});
  args::ValueFlag<string> extentsArg(parser, "Extents", "4 comma separated values; min lat, min long, max lat, max long", {'e'});
  args::ValueFlag<string> refPointArg(parser, "RefPoint", "2 comma separated values; latLng coords. To be used as point of origin for geometry ", {'p'});
  args::ValueFlag<string> s2CellArg(parser, "S2Cell", "S2 Cell Id as hex string. Filters nodes and ways only somepart inside S2 Cell", {'s', "s2"});
  args::ValueFlag<float> uvScaleArg(parser, "UV Scale", "Scale UV set. UV set rounds to nearest 1.0 for quad nearest to given scale. Default parameter of zero omits UV set altogether.", {'u', "uv"});

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
    std::cerr << e.what() << endl;
    std::cerr << parser;
    return 1;
  }
  catch (args::ValidationError e)
  {
    std::cerr << e.what() << endl;
    std::cerr << parser;
    return 1;
  }
  if (!inputFileArg || !outputFileArg)
  {
    cout << parser;
    return 0;
  }

  if (consolidateArg)
  {
    int choice = args::get(consolidateArg);
    if (choice < 0 || choice > 2)
    {
      cout << "Consolidation choice can only be 0, 1 0r 2" << endl;
      cout << parser;
      exit(1);
    }
    assimpWriter.setMeshGranularity(static_cast<AssimpWriter::MeshGranularity>(choice));
  }

  if (exportZUpArg)
  {
    GeomConvert::zUp = true;
  }

  if (convertCEF)
  {
    ConvertLatLngToCoords::UseCenterEarthFixed = true;
  }

  string outputFile = args::get(outputFileArg);
  auto outExt = getFileExt(outputFile);

  if (!assimpWriter.checkFormat(outExt))
  {
    cout << "Couldnt find filetype for extension '" << outExt << "', the avialable types are " << assimpWriter.formatsAvailableStr() << endl;
    return 1;
  }

  vector<glm::vec2> groundCorners;
  ViewFilterList viewFilters;

  //the originLocation will decide the point of origin for the exported geometry file.
  //if it's not specified by the refPoint cmd line arg it is taken from the bottom left
  //corner of the box, which in turn can be decided by the extents argument,
  // or is otherwise taken from the bounds of the input file.
  osmium::Location originLocation;

  osmium::Box box;

  if (extentsArg)
  {
    try
    {
      string extentsStr = args::get(extentsArg);

      box = osmiumBoxFromString(extentsStr);

      originLocation = box.bottom_left();
      ConvertLatLngToCoords::setRefPoint(originLocation);

      viewFilters.push_back(make_shared<BoundFilter>(box));

      if (groundArg)
      {
        groundCorners = cornersFromBox(box);
      }
    }
    catch (std::invalid_argument)
    {
      cout << "Failed to parse extents string '" << args::get(extentsArg) << "'" << endl;
      std::exit(1);
    }
  }

  if (refPointArg)
  {
    try
    {

      originLocation = refPointFromArg(args::get(refPointArg));
      ConvertLatLngToCoords::setRefPoint(originLocation);
    }
    catch (std::invalid_argument)
    {
      cout << "failed to parse ref point string '" << args::get(refPointArg) << "'" << endl;
      std::exit(1);
    }
  }

  if (s2CellArg)
  {
    try
    {

      string s2CellStr = args::get(s2CellArg);
      if (s2CellStr.size() < 16)
      {
        int addZeroes = 16 - s2CellStr.size();
        for (int i = 0; i < addZeroes; i++)
          s2CellStr += "0";
      }

      uint64_t s2cellId = S2Util::getS2IdFromString(s2CellStr);

      viewFilters.push_back(make_shared<S2CellFilter>(s2cellId));

      S2Util::LatLng latLng = S2Util::getS2Center(s2cellId);
      originLocation = {std::get<1>(latLng), std::get<0>(latLng)};
      ConvertLatLngToCoords::setRefPoint(originLocation);

      if (groundArg)
      {
        auto cornerCoords = S2Util::getS2Corners(s2cellId);

        groundCorners.resize(4);
        for (int i = 0; i < 4; i++)
        {
          osmium::geom::Coordinates coord = ConvertLatLngToCoords::to_coords({std::get<1>(cornerCoords[i]), std::get<0>(cornerCoords[i])});
          groundCorners[i] = {coord.x, coord.y};
        }
      }
    }
    catch (const std::exception &)
    {

      cout << "failed to parse S2 Cell id string '" << args::get(s2CellArg) << "', is it a <=16 character hex string?" << endl;
      std::exit(1);
    }
  }

  if (uvScaleArg)
  {
    GeomConvert::texCoordScale = args::get(uvScaleArg);
  }

  vector<string> inputFiles = getInputFiles(args::get(inputFileArg));

  osmium::Box filesBox;

  int filter = OSMFeature::BUILDING;
  if (highwayArg)
  {
    filter |= OSMFeature::HIGHWAY;
  }

  viewFilters.push_back(make_shared<TypeFilter>(filter));

  SceneConstruct sceneConstruct(viewFilters);

  for (auto &inputFile : inputFiles)
  {
    cout << inputFile << endl;

    string inputExt = inputFile.substr(inputFile.size() - 3, 3);

    if (inputExt != "osm" && inputExt != "pbf")
    {
      cout << "Input format must either be 'osm' or 'pbf'\n input file is " << inputFile << endl;
      exit(1);
    }

    index_type index;

    location_handler_type locationHandler{index};

    try
    {

      osmium::io::Reader osmFileReader{inputFile, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way};

      if (!box.valid())
      {
        osmium::io::Header header = osmFileReader.header();
        box = header.box();

        if (!box.valid() && !refPointArg)
        {
          cout << "No bounds found in osm file and no RefPoint specified, aborting" << endl;
          exit(1);
        }
        filesBox.extend(box);
      }

      //if the refPoint was not set via commandline take the bottom left of the first input file specified
      if (!originLocation)
      {
        originLocation = box.bottom_left();
        ConvertLatLngToCoords::setRefPoint(originLocation);
      }

      // this is where it all happens
      osmium::apply(osmFileReader, locationHandler, sceneConstruct);

      cout << "Ways Exported: " << sceneConstruct.wayCount() << endl;
    }
    catch (const std::system_error &err)
    {
      cout << "Failed to read " << inputFile << ", " << err.what() << endl;
      continue;
    }
  }

  if (groundArg)
  {
    if (groundCorners.size() == 0)
    {
      groundCorners = cornersFromBox(filesBox);
    }
    sceneConstruct.addGround(groundCorners);
  }

  if (sceneConstruct.wayCount())
  {
    SceneConstruct::OutputConfig config{exportZUpArg, args::get(uvScaleArg)};
    if (AI_SUCCESS != sceneConstruct.write(outputFile, assimpWriter, config))
    {
      cout << "Failed to write out to '" << outputFile << "', " << assimpWriter.exporterErrorStr() << endl;
      return 1;
    }
  }
  else
  {
    cout << "No relevant features found, nothing to write" << endl;
  }

  double elapsed = (std::clock() - start) / (double)CLOCKS_PER_SEC;

  cout << "Time Taken : " << elapsed << endl;

  return 0;
}
