#include "osmfeature.h"
#include "viewfilter.h"
#include "assimpwriter.h"
#include <osmium/handler.hpp>
#include <map>

namespace GeoUtils
{
  class SceneConstruct : public osmium::handler::Handler
  {
  public:
    SceneConstruct(const ViewFilterList &filters);

    //osmium::handler::Handler
    void way(const osmium::Way &way);
    void node(const osmium::Node &node);

    void addGround(const std::vector<glm::vec2> &groundCorners);

    size_t wayCount()
    {
      return mFeatures.size();
    }

    struct OutputConfig
    {
      bool mZUp = true;
      float mTexCoordScale = 0.0f;
    };

    //write file using assimp, returns assimp ret code
    int write(const std::string &outFilePath, AssimpWriter &writer, const OutputConfig &config);

  protected:
    const ViewFilterList &mFilters;
    std::map<std::string, glm::vec3> mMatColors;
    std::vector<OSMFeature> mFeatures;
  };

}