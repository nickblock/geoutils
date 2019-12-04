
#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/visitor.hpp>
#include <osmium/util/memory.hpp>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <fstream>
#include <experimental/filesystem>

#include "args.hxx"
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <set>

#include "main.h"
#include "osmsplitwriter.h"
#include "osmsplitconfig.h"
#include "mapsplit.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::thread;
using std::mutex;
using std::vector;
using std::set;
using std::ofstream;

using GeoUtils::MapHandler;
using GeoUtils::OSMSplitWriter;
using GeoUtils::OSMSplitConfigPtr;
using GeoUtils::OSMSplitConfig;
using GeoUtils::OSMConfigList;

namespace fs = std::experimental::filesystem;


using IdSet = set<osmium::unsigned_object_id_type>;


std::clock_t startTime;  
uint64_t numLocs = 0;

string constructOutPrefix(const string& inputFile) {

  string fileName = inputFile;
  
  int lastSlash = fileName.find_last_of("/");
  if(lastSlash != string::npos)
    fileName = fileName.substr(lastSlash+1, fileName.size());
  
  int spotPos = fileName.find_first_of(".");
  if(spotPos != string::npos)
    fileName = fileName.substr(0, spotPos);

  return fileName;
}

string addSlash(const std::string dirName) {

  string outDir = dirName;
  if(outDir[outDir.size()-1] != '/') {
    outDir += string("/");
  }
  return outDir;
}

string fileNameFromPath(const string& path) {

  string nameOnly = path;
  int slashPos = path.find_last_of("/");
  if(slashPos != string::npos) {
    nameOnly = nameOnly.substr(slashPos+1, nameOnly.size());
  }

  return nameOnly;
}

string getDirFromPath(const string& path) {

  string pathOnly = path;
  int slashPos = path.find_last_of("/");
  if(slashPos != string::npos) {
    pathOnly = pathOnly.substr(0, slashPos+1);
  }
  return pathOnly;
}

int getMemUse() {
  osmium::MemoryUsage mem;
  return mem.current();
}

int getElapsedTime() {

  double now = std::clock();
  return (now - startTime)/(double)CLOCKS_PER_SEC;
}

void printMemTimeUpdate() {
  cout << "\r" << "elapsed " << getElapsedTime() << " mem : " << getMemUse() << endl;
}

void writeConfigFile(std::string configFileName, OSMSplitConfigPtr config) {

  ofstream ofs(configFileName);
  rapidjson::OStreamWrapper osw(ofs);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
  rapidjson::Document configDoc;
  rapidjson::Document::AllocatorType& a = configDoc.GetAllocator();
  configDoc.SetObject().AddMember("osmsplit", config->toJson(a), a);
  configDoc.Accept(writer);
}

void processOSMFile(std::string inputFileName, std::string outDir, OSMSplitConfigPtr& config, SplitOptions options)
{
  string outFileNamePrefix = constructOutPrefix(inputFileName);

  if(options.updateOnly) {
    string potentialOutFile = outDir + outFileNamePrefix;
    for(int i=0; i<options.depthLevels; i++) 
      potentialOutFile += string("0");

    potentialOutFile += OSMSplitConfig::suffix();

    if(fs::exists(potentialOutFile)) {
      auto newTime = fs::last_write_time(potentialOutFile);
      auto inputTime = fs::last_write_time(inputFileName);

      if(newTime > inputTime) {
        cout << "Skipping existing outputs like '" << potentialOutFile << "'" << endl;
        return;
      }
    }
  }

  osmium::io::File inputOsFile{inputFileName}; 
  osmium::io::Reader reader{inputOsFile, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way}; 
  cout << "Procssing File " << inputFileName << endl;

  if(config == nullptr) {
    config = std::make_shared<OSMSplitConfig>(reader.header().box(), outFileNamePrefix);
  }
  
  NodeLocatorMap nodeLocatorStore;

  if(!reader.header().box()) {
    cout << "No extents in header" << endl;
    return;
  }
  MapHandler<uint32_t, 1024> mapHandler(config, options.sampleRate, nodeLocatorStore);

  osmium::apply(reader, mapHandler);

  cout << "Read Locations" << endl;
  printMemTimeUpdate();
  cout << "nodes " << nodeLocatorStore.size() << endl << endl;

  numLocs = nodeLocatorStore.size();

  mapHandler.finish(outDir + config->getFileName() + ".split.png", options.depthLevels);

  cout << "Writing out" << endl;
  printMemTimeUpdate();

  OSMSplitWriter osm_writer( 
      config,
      inputFileName,
      outDir,
      nodeLocatorStore,
      options.threadNum,
      mapHandler.numWays()
    );
}
void processConfigFile(std::string inputFileName, std::string outDir, OSMSplitConfigPtr& config, SplitOptions options)
{
  string inputDir = getDirFromPath(inputFileName);

  std::stringstream ss;
  std::ifstream file(inputFileName);
  ss << file.rdbuf();
  file.close();
  rapidjson::Document d;

  if(!d.Parse<0>(ss.str().c_str()).HasParseError() && d.HasMember("osmsplit")) 
  { 
    config = std::make_shared<OSMSplitConfig>(d["osmsplit"]);
  
    OSMConfigList configList = config->getLeafNodes();
    for(auto leaf : configList) {

      string inputFilePath = inputDir + leaf->getFileName();
      
      processOSMFile(
              inputFilePath,
              outDir, 
              leaf,
              options);

      if(options.deleteInputFiles) {
        fs::remove(inputFilePath);
      }
    }
  }
  else {
    cout << "Faliled to parse config file '" << inputFileName << "'" << endl;

    return;
  }
}

int main(int argi, char** argc) {

  startTime = std::clock();

  args::ArgumentParser          parser("osmsplit. Take a single large osm file and split it in half N times", 
    "You must at least specify one input, an output directory, and the number of splits to go down to, ");

  args::ValueFlag<std::string>  inputFileArg(parser, "*.osm|*.pbf", "Specify input .osm file", {'i'});
  args::ValueFlag<std::string>  outputDirArg(parser, "/", "Specify output directory", {'o'});
  args::ValueFlag<int>          levelsArg(parser, "1", "Specify number of splits wanted", {'l'});
  args::ValueFlag<int>          sampleRateArg(parser, "s", "Sample Rate", {'s'});
  args::ValueFlag<int>          maxThreadsArg(parser, "t", "Max Threads", {'t'});
  args::Flag                    updateOnlyArg(parser, "u", "Don't redo existing output files if input file is older", {'u'});
  args::Flag                    deleteInputFilesArg(parser, "d", "Delete input files", {'d'});

  // couldn't get splitwriter to work with normal osm files
  // args::Flag                    outputXMLFormat(parser, "x", "Output to xml format (protobuf is default)", {'x'});

  try
  {
      parser.ParseCLI(argi, argc);
  }
  catch (args::Help)
  {
      std::cout << parser;
      return 1;
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
  if(!inputFileArg || !outputDirArg || !levelsArg) {

      cout << parser;
      return 1;
  }

  SplitOptions options;

  if(sampleRateArg) {
    options.sampleRate = args::get(sampleRateArg);
  }
  if(levelsArg) {
    options.depthLevels = args::get(levelsArg);
  }

  if(maxThreadsArg) {
    options.threadNum = args::get(maxThreadsArg);
  }

  if(deleteInputFilesArg) {
    options.deleteInputFiles = args::get(deleteInputFilesArg);
  }

  if(updateOnlyArg) {
    options.updateOnly = args::get(updateOnlyArg);
  }

  // if(outputXMLFormat) {
  //   OSMSplitConfig::setOutputSuffix(".osm");
  // }
  
  try {

    string outDir = addSlash(args::get(outputDirArg));
    string inputFileName = args::get(inputFileArg);

    OSMSplitConfigPtr config;

    string configFileName;

    int remainingDepthLevels = options.depthLevels;

    while(remainingDepthLevels) {

      if(remainingDepthLevels > 8) {
        remainingDepthLevels -= 8;
        options.depthLevels = 8;
      }
      else {
        options.depthLevels = remainingDepthLevels;
        remainingDepthLevels = 0;
      }
      if(inputFileName.substr(inputFileName.size()-3) == ".js") {

        processConfigFile(inputFileName, outDir, config, options);

        configFileName = outDir + fileNameFromPath(inputFileName);
    
        writeConfigFile(configFileName, config);

        if(remainingDepthLevels) {
          inputFileName = configFileName;
          options.deleteInputFiles = true;
        }
      }
      else {

        processOSMFile(inputFileName, outDir, config, options);

        configFileName = outDir + constructOutPrefix(inputFileName) + "_conf.js";

        writeConfigFile(configFileName, config);

        if(options.deleteInputFiles) {
          fs::remove(inputFileName);
        }

        if(remainingDepthLevels) {
          inputFileName = configFileName;
          options.deleteInputFiles = true;
        }
      }
    }

  } catch (std::bad_alloc&) {

    osmium::MemoryUsage mem;
    
    cout << "Bad alloc: max mem=" << mem.peak() << endl;

  } catch (const std::exception& ex) {
    
    cout << "Exception " << ex.what() << endl;
  }

  printMemTimeUpdate();
  
  return 0;
}
