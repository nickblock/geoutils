#include "args.hxx"
#include "tinyformat.h"
#include <stdint.h>
#include <regex>
#include "s2/s2cell.h"
#include "s2/s2latlng.h"
#include "s2/s2cell_id.h"

using std::string;
using std::cout;
using std::endl;
using std::cerr;

int main(int argi, char** argv)
{
  args::ArgumentParser  parser("s2util. Return coordinates of teh center of an s2 cell.");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Positional<string>  inputS2Cell(parser, "0x0", "Enter a string containing an 8 digit hex number representing a valid s2 cell id");

  try
  {
      parser.ParseCLI(argi, argv);
  }
  catch (args::Help)
  {
      std::cout << parser;
      std::exit(1);
  }

  if(args::get(inputS2Cell).size() == 0) {
    
      std::cout << parser;
      std::exit(1); 
  }

  try {
    
    std::smatch res; 
    std::regex reg(".*([0-9a-f]{16}).*");
    std::regex_match (args::get(inputS2Cell), res, reg);
    if (res.size() <= 1) {
      cerr << "failed to find cell id in " << endl;
      std::exit(1);
    }

    uint64_t number = std::stoul(res[1], nullptr, 16);

    const S2CellId cellId(number);

    if(cellId.is_valid()) {
      const S2LatLng cellCenter = cellId.ToLatLng();
        
      cout << cellCenter.lat() << ", " << cellCenter.lng() << endl;
    }
    else {
      std::cerr << "Invalid cell Id (" << number << ")" << endl;
    }
  }
  catch (const std::exception& ex) {
    std::cerr << "Couldn't decode input as hex number. " << ex.what() << endl;
    std::exit(1);
  }

  return 0;
}