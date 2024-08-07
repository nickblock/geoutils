#include "sceneconstruct.h"
#include "assimp/mesh.h"
#include "assimpwriter.h"
#include "common.h"
#include "geometry.h"
#include "ground.h"
#include <iostream>

using std::cout;
using std::endl;
using std::string;

namespace GeoUtils {
void sanitizeName(string &name) { EngineBlock::replaceAll(name, "&", "&amp;"); }
SceneConstruct::SceneConstruct(const ViewFilterList &filters)
    : mFilters(filters) {
  std::locale::global(std::locale(""));

  mMatColors["ground"] = glm::vec3(149 / 255.f, 174 / 255.f, 81 / 255.f);
  mMatColors["highway"] = glm::vec3(81 / 255.f, 149 / 255.f, 174 / 255.f);
  mMatColors["water"] = glm::vec3(0.2, 0.4, 0.8);
  mMatColors["green"] = glm::vec3(0.2, 0.7, 0.1);
  mMatColors["red"] = glm::vec3(0.8, 0.1, 0.2);
  mMatColors["default"] = glm::vec3(174 / 255.f, 81 / 255.f, 149 / 255.f);
  mMatColors["building"] = glm::vec3(0.9, 0.1, 0.7);
}

void SceneConstruct::way(const osmium::Way &way) {
  for (const auto &filter : mFilters) {
    if (!filter->include(way)) {
      return;
    }
  }

  OSMFeature feature(way);

  if (feature.isValid()) {
    mFeatures.push_back(feature);
  }
}

void SceneConstruct::node(const osmium::Node &node) {}
void SceneConstruct::addGround(const std::vector<glm::vec2> &groundCorners) {
  mGround = std::make_unique<Ground>(groundCorners);
}
int SceneConstruct::write(const std::filesystem::path &outFilePath,
                          AssimpWriter &writer, const OutputConfig &config) {
  Geometry::zUp = config.mZUp;
  Geometry::texCoordScale = config.mTexCoordScale;

  int retVal = 0;

  int count = 0;
  for (auto &feature : mFeatures) {
    try {
      aiMesh *mesh = nullptr;

      // if it's something that wants turning into a 3d mesh
      if (feature.type() & (OSMFeature::BUILDING | OSMFeature::WATER) &&
          feature.type() & OSMFeature::CLOSED) {
        mesh =
            Geometry::extrude2dMesh(feature.coords(), feature.height(), count)
                .simpleMesh();
      }

      // if it's something that wants turning into a polygon spline
      else if (feature.type() & OSMFeature::HIGHWAY) {
        mesh = Geometry::meshFromLine(feature.coords(), OSMFeature::RoadWidth,
                                      count)
                   .simpleMesh();
      }

      // if we made either kind of mesh successfully
      if (mesh) {
        string materialName = "default";

        if (feature.type() & OSMFeature::BUILDING) {
          materialName = "building";
        } else if (feature.type() & OSMFeature::HIGHWAY) {
          materialName = "highway";
        } else if (feature.type() & OSMFeature::WATER) {
          materialName = "water";
        }

        if (mesh->mNumFaces == 0 || mesh->mFaces[0].mNumIndices == 0) {
          continue;
        }

        mesh->mMaterialIndex =
            writer.addMaterial(materialName, mMatColors[materialName]);

        string nameSanitize = feature.name();
        sanitizeName(nameSanitize);

        writer.addMesh(mesh, nameSanitize, nullptr);

        count++;
      } else {

        static bool failed = true;
        if (failed) {
          cout << "Failed to import some buildings, eg " << feature.name()
               << endl;
          failed = false;
        }
      }
    } catch (std::out_of_range) {

      static bool once = true;
      if (once) {
        cout << "Failed to decode some nodes" << endl;
        once = false;
      }
      retVal = -1;
    } catch (std::runtime_error &err) {

      static bool once = true;
      if (once) {
        cout << err.what() << endl;
        once = false;
      }
      retVal = -1;
    }
  }

  // if (mGround) {

  //   for (auto &mesh : writer.meshes()) {
  //     ground.addFootPrint(GeomConvert::getFootprint(
  //         {(const glm::vec3 *)mesh->mVertices, mesh->mNumVertices}));
  //   }

  //   aiMesh *mesh = ground.getMesh();
  //   if (mesh) {
  //     mesh->mMaterialIndex = writer.addMaterial("ground",
  //     mMatColors["ground"]); writer.addMesh(mesh, "ground");
  //   } else {
  //     retVal = -1;
  //   }
  // }

  if (writer.write(outFilePath) == 0 && retVal == 0) {
    return 0;
  } else {
    return -1;
  }
}
} // namespace GeoUtils