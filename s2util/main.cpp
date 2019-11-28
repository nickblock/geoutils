#include "args.hxx"
#include "tinyformat.h"
#include "s2util.h"
#include <stdint.h>

using std::string;
using std::cout;
using std::endl;
using std::cerr;

using GeoUtils::S2Util;

int main(int argi, char** argv)
{
  args::ArgumentParser  parser("s2util. Return coordinates of the center of an s2 cell.");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Positional<string>  inputS2Cell(parser, "0x0", "Enter a string containing an 16 digit hex number representing a valid s2 cell id");
  args::Flag gpsArg(parser, "GPS Coordinates", "Returns center of S2 cell as Lat Lng", {"c"});
  args::Flag parentArg(parser, "Parent Id", "Returns parent cell id of input cell", {"p"});
  
  try
  {
      parser.ParseCLI(argi, argv);
  }
  catch (args::Help)
  {
      std::cout << parser;
      std::exit(1);
  }

  if(args::get(inputS2Cell).size() == 0 || !(gpsArg || parentArg)) {
    
      std::cout << parser;
      std::exit(1); 
  }

  try {
    
    uint64_t number = S2Util::getS2IdFromString(args::get(inputS2Cell));

    const S2CellId cellId(number);

    if(cellId.is_valid()) {
      
      if(gpsArg) {
        
        auto cellCenter = S2Util::getS2Center(number);
          
        cout << std::get<0>(cellCenter) << "," << std::get<1>(cellCenter) << " ";
      }

      if(parentArg) {
        uint64_t parentId = S2Util::getParent(number);

        cout << std::hex << parentId << " ";
      }
    }
    else {
      throw std::invalid_argument(std::string("Invalid cell Id (") + std::to_string(number) + std::string(")"));
    }

  }
  catch (const std::exception& ex) {
    std::cerr << "Couldn't decode input as hex number. " << ex.what() << endl;
    std::exit(1);
  }

  return 0;
}