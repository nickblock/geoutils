#include "utils.h"
#include "s2util.h"
#include "convertlatlng.h"
#include "osmdata.h"
#include "geomconvert.h"
#include "assimpconstruct.h"

#include "assimp/scene.h"

#include <glm/vec3.hpp>

#include <osmium/geom/coordinates.hpp>

#include <iostream>
#include <sstream>

namespace GeoUtils
{
  osmium::Box osmiumBoxFromString(string extentsStr)
  {
    osmium::Box box;

    osmium::Location locMin, locMax;

    int commaPos = extentsStr.find_first_of(",");
    locMin.set_lon(std::stod(extentsStr.substr(0, commaPos)));
    extentsStr = extentsStr.substr(commaPos + 1, extentsStr.size());

    commaPos = extentsStr.find_first_of(",");
    locMin.set_lat(std::stod(extentsStr.substr(0, commaPos)));
    extentsStr = extentsStr.substr(commaPos + 1, extentsStr.size());

    commaPos = extentsStr.find_first_of(",");
    locMax.set_lon(std::stod(extentsStr.substr(0, commaPos)));
    extentsStr = extentsStr.substr(commaPos + 1, extentsStr.size());

    locMax.set_lat(std::stod(extentsStr));

    box.extend(locMin);
    box.extend(locMax);

    return box;
  }

  vector<string> getInputFiles(string input)
  {
    vector<std::string> tokens;
    string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, ','))
    {
      if (token.size())
      {
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
    refPointStr = refPointStr.substr(commaPos + 1, refPointStr.size());
    location.set_lon(std::stod(refPointStr.substr(0, commaPos)));

    return location;
  }

  std::string getFileExt(const std::string filename)
  {
    int dotPos = filename.find_last_of('.');

    return filename.substr(dotPos + 1);
  }

  void parentNodesToS2Cell(uint64_t s2cellId, OSMDataImport &importer)
  {
    S2Util::LatLng s2LatLng = S2Util::getS2Center(s2cellId);
    osmium::Location s2CellCenterLocation = osmium::Location(std::get<1>(s2LatLng), std::get<0>(s2LatLng));

    //the coords are given relative to the originLocation
    const osmium::geom::Coordinates s2CellCenterCoord = ConvertLatLngToCoords::to_coords(s2CellCenterLocation);
    ConvertLatLngToCoords::setRefPoint(s2CellCenterLocation);

    aiNode *s2CellParentAINode = new aiNode(std::to_string(s2cellId));

    aiVector3t<float> asm_pos(s2CellCenterCoord.x, 0.0f, s2CellCenterCoord.y);
    aiMatrix4x4t<float>::Translation(asm_pos, s2CellParentAINode->mTransformation);

    importer.setParentAINode(s2CellParentAINode);
  }

  vector<glm::vec2> cornersFromBox(const osmium::Box &box)
  {
    vector<glm::vec2> groundCorners(4);

    osmium::geom::Coordinates bottom_left = ConvertLatLngToCoords::to_coords(box.bottom_left());
    osmium::geom::Coordinates top_right = ConvertLatLngToCoords::to_coords(box.top_right());
    osmium::geom::Coordinates bottom_right(top_right.x, bottom_left.y);
    osmium::geom::Coordinates top_left(bottom_left.x, top_right.y);

    groundCorners[0] = {bottom_left.x, bottom_left.y};
    groundCorners[1] = {top_right.x, top_right.y};
    groundCorners[2] = {bottom_right.x, bottom_right.y};
    groundCorners[3] = {top_left.x, top_left.y};

    return groundCorners;
  }

  void addGround(const vector<glm::vec2> &groundCorners, bool zup, AssimpConstruct &assimpConstruct)
  {
    float groundDepth = 0.1;
    aiMesh *mesh = GeomConvert::extrude2dMesh(groundCorners, groundDepth);
    aiNode *parent = new aiNode;
    aiMatrix4x4::Translation(zup ? aiVector3D(0.0, 0.0, -groundDepth) : aiVector3D(0.0, -groundDepth, 0.0), parent->mTransformation);
    mesh->mMaterialIndex = assimpConstruct.addMaterial("ground", glm::vec3(149 / 255.f, 174 / 255.f, 81 / 255.f));
    assimpConstruct.addMesh(mesh, "ground", parent);
  }

  //boudning box functions

  BBox::BBox()
  {
    mMin.x = std::numeric_limits<float>::max();
    mMin.y = std::numeric_limits<float>::max();
    mMin.z = std::numeric_limits<float>::max();
    mMax.x = std::numeric_limits<float>::min();
    mMax.y = std::numeric_limits<float>::min();
    mMax.z = std::numeric_limits<float>::min();
  }

  void BBox::add(const glm::vec3 &p)
  {
    mMin = min(mMin, p);
    mMax = max(mMax, p);
  }
  void BBox::add(const BBox &bb)
  {
    mMin = min(mMin, bb.mMin);
    mMax = max(mMax, bb.mMax);
  }

  auto BBox::transform(const glm::mat4 &mat) -> BBox
  {
    BBox tBbox;

    tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMin.y, mMin.z, 1.0)));
    tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMax.y, mMax.z, 1.0)));
    tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMin.y, mMin.z, 1.0)));
    tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMax.y, mMin.z, 1.0)));
    tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMin.y, mMax.z, 1.0)));
    tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMax.y, mMax.z, 1.0)));
    tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMin.y, mMax.z, 1.0)));
    tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMax.y, mMin.z, 1.0)));

    return tBbox;
  }
} // namespace GeoUtils