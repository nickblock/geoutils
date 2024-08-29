#include "assimpwriter.h"
#include "ground.h"
#include "osmfeature.h"
#include "roadgraph.h"
#include "viewfilter.h"
#include <map>
#include <osmium/handler.hpp>

namespace GeoUtils {

class Ground;
class SceneConstruct : public osmium::handler::Handler {
public:
  SceneConstruct(const ViewFilterList &filters);
  virtual ~SceneConstruct();

  // osmium::handler::Handler
  void way(const osmium::Way &way);
  void node(const osmium::Node &node);

  void addGround(const std::vector<glm::vec2> &groundCorners);

  size_t wayCount() { return mFeatures.size(); }

  struct OutputConfig {
    bool mZUp = true;
    float mTexCoordScale = 0.0f;
  };

  // write file using assimp, returns assimp ret code
  int write(const std::filesystem::path &outFilePath, AssimpWriter &writer,
            const OutputConfig &config);

protected:
  const ViewFilterList &mFilters;
  std::map<std::string, glm::vec3> mMatColors;

  // all OSM features are extracted into this list, before then being converted
  // to geometry and saved out
  std::vector<OSMFeature> mFeatures;

  std::unique_ptr<Ground> mGround;
  std::unique_ptr<RoadGraph<osmium::NodeRef>> mRoadGraph;
};

} // namespace GeoUtils