#include "args.hxx"
#include "tinyformat.h"
#include <stdint.h>

using std::string;
using std::cout;
using std::endl;

int main(int argi, char** argv)
{
  args::ArgumentParser  parser("s2util. Assess s2 cell");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Positional<string>  inputS2Cell(parser, "0x0", "Enter S2 Cell as Hex number");

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

  uint64_t cellId;

  try {
    cellId = std::stoul(args::get(inputS2Cell), nullptr, 16);
  }
  catch (std::exception ex) {
    std::cerr << "Couldn't decode input as hex number, " << ex.what() << endl;
    std::exit(1);
  }

  cout << cellId << endl;

  return 0;
}