#include "args.hxx"
#include "tinyformat.h"
#include "s2util.h"
#include <stdint.h>

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
    
    uint64_t number = S2Util::getS2IdFromString(args::get(inputS2Cell));
    
    if (number < 1) {
      cerr << "failed to find cell id in " << endl;
      std::exit(1);
    }

    const S2CellId cellId(number);

    if(cellId.is_valid()) {

      auto cellCenter = S2Util::getS2Center(number);
        
      cout << std::get<0>(cellCenter) << ", " << std::get<1>(cellCenter) << endl;
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