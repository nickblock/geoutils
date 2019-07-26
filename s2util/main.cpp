#include "args.hxx"
#include "tinyformat.h"
#include <stdint.h>
#include <regex>

using std::string;
using std::cout;
using std::endl;
using std::cerr;

int main(int argi, char** argv)
{
  args::ArgumentParser  parser("s2util. Return coordinates of teh center of an s2 cell.");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Positional<string>  inputS2Cell(parser, "0x0", "Enter a string containing an 8 digit hex number");

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
    
    std::smatch res; 
    std::regex reg(".*([0-9a-f]{8}).*");
    std::regex_match (args::get(inputS2Cell), res, reg);
    if (res.size() > 1)
      cout << "string object matched " << res[1] << endl;
    else 
      cerr << "failed to match" << endl;

    cellId = std::stoul(res[1], nullptr, 16);
  }
  catch (std::invalid_argument ex) {
    std::cerr << "Couldn't decode input as hex number." << endl;
    std::exit(1);
  }

  cout << cellId << endl;

  return 0;
}