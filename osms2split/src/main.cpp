
/*

  EXAMPLE osmium_pub_names

  Show the names and addresses of all pubs found in an OSM file.

  DEMONSTRATES USE OF:
  * file input
  * your own handler
  * access to tags

  SIMPLER EXAMPLES you might want to understand first:
  * osmium_read
  * osmium_count

  LICENSE
  The code in this example file is released into the Public Domain.

*/

#include <cstdlib>  // for std::exit
#include <cstring>  // for std::strncmp
#include <iostream> // for std::cout, std::cerr
#include "args.hxx"

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

// We want to use the handler interface
#include <osmium/handler.hpp>

// For osmium::apply()
#include <osmium/visitor.hpp>

#include <osmium/index/node_locations_map.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

#include "s2splitter.h"

using std::string;
using std::cout;
using std::endl;
using std::vector;

using NodeLocatorMap = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<NodeLocatorMap>;

vector<string> getKeysOfInterest(string input)
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
int main(int argi, char* argv[]) {

  args::ArgumentParser  parser("osmsplits2. Take a single large osm file and split it into files defined by s2 cells", 
    "You must at least specify one input osm/som.pbf file, an output directory, and the s2 level to split dowm to");

  args::ValueFlag<string>  inputFileArg(parser, "*.osm|*.pbf", "Specify input .osm file", {'i'});
  args::ValueFlag<string>  outputDirArg(parser, "/", "Specify output directory", {'o'});
  args::ValueFlag<string>  keysOfInterestArg(parser, "key1,key2", "Comma separated keys to search for. If not set everything is included", {'k'});
  args::ValueFlag<int>     s2LevelArg(parser, "1", "Specify number of splits wanted", {'l'});
  args::Flag               outputXmlArg(parser, "x", "Output xml (.osm), default is pbf", {'x'});

  try
  {
      parser.ParseCLI(argi, argv);
  }
  catch (args::Help)
  {
      std::cout << parser;
      std::exit(1);
  }

  if(args::get(inputFileArg).size() == 0 || args::get(outputDirArg).size() == 0) 
  {
      std::cout << parser;
      std::exit(1);
  }
  
  try {

      NodeLocatorMap nodeLocatorStore;

      location_handler_type location_handler(nodeLocatorStore);

      int s2Level = args::get(s2LevelArg);

      S2Splitter s2Splitter(s2Level);

      if(args::get(outputDirArg).size() > 0) {
        s2Splitter.setOutputDirectory(args::get(outputDirArg));
      }
      if(args::get(outputXmlArg)) {
        s2Splitter.setOutputXml(true);
      }
      if(args::get(keysOfInterestArg).size())
      {
        vector<string> keysOfInterest = getKeysOfInterest(args::get(keysOfInterestArg));
        for(auto& key : keysOfInterest) {
          s2Splitter.setKeyOfInterest(key);
        }
      }

      osmium::io::Reader reader{args::get(inputFileArg), osmium::osm_entity_bits::node | osmium::osm_entity_bits::way};

      osmium::apply(reader, location_handler, s2Splitter);

      cout << "done" << endl;

  } catch (const std::exception& e) {
      // All exceptions used by the Osmium library derive from std::exception.
      std::cerr << e.what() << '\n';
      std::exit(1);
  }
}

