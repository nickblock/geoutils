#include "args.hxx"
#include "tinyformat.h"
#include "s2util.h"
#include <stdint.h>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

using GeoUtils::S2Util;

int main(int argi, char **argv)
{
  args::ArgumentParser parser("s2util. Return coordinates of the center of an s2 cell.");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Positional<string> inputS2Cell(parser, "0x0", "Enter a string containing an 16 digit hex number representing a valid s2 cell id");
  args::Flag gpsArg(parser, "GPS Coordinates", "Returns center of S2 cell as Lat Lng", {'c'});
  args::Flag parentArg(parser, "Parent Id", "Returns parent cell id of input cell", {'p'});
  args::ValueFlag<std::string> cartCoords(parser, "Cartesian", "Enter a origin LatLon (comma separated, no spaace), coord to return a relative Vec3 of the s2 cell's center, calculated using ECEF", {'o'});

  try
  {
    parser.ParseCLI(argi, argv);
  }
  catch (args::Help)
  {
    std::cout << parser;
    std::exit(1);
  }

  if (args::get(inputS2Cell).size() == 0)
  {
    std::cout << parser;
    std::exit(1);
  }

  try
  {

    uint64_t number = S2Util::getS2IdFromString(args::get(inputS2Cell));

    const S2CellId cellId(number);

    if (cellId.is_valid())
    {

      if (gpsArg || cartCoords)
      {
        auto cellCenter = S2Util::getS2Center(number);

        if (gpsArg)
        {
          cout << "Cell Center = " << std::get<0>(cellCenter) << "," << std::get<1>(cellCenter) << endl;
        }
        if (cartCoords)
        {
          auto originLatLng = S2Util::parseLatLonString(args::get(cartCoords));

          auto position = S2Util::LLAToCartesian(originLatLng, cellCenter);

          cout << "Cell Position = " << std::get<0>(position) << "," << std::get<1>(position) << endl;
        }
      }

      if (parentArg)
      {
        uint64_t parentId = S2Util::getParent(number);

        cout << "Parent ID = " << std::hex << parentId << endl;
      }
    }
    else
    {
      throw std::invalid_argument(std::string("Invalid cell Id (") + std::to_string(number) + std::string(")"));
    }
  }
  catch (const std::invalid_argument& ex)
  {
    std::cerr << "Couldn't parse some string input. " << ex.what() << endl;
    std::exit(1);
  }
  catch (const std::exception &ex)
  {
    std::cerr << "Couldn't decode input as hex number. " << ex.what() << endl;
    std::exit(1);
  }

  return 0;
}