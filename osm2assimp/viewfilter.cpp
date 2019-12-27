#include "viewfilter.h"
#include "s2/s2cell.h" 
#include "s2/s2latlng.h"
#include "s2/s2point.h"  

namespace GeoUtils {

using std::make_unique;

TypeFilter::TypeFilter(int filter) 
: mFilter(filter)
{

}

bool TypeFilter::include(const osmium::Way& way) 
{
  for(const auto& node: way.nodes()) {

  }
  return false;
}

BoundFilter::BoundFilter(const osmium::Box& box) 
: mBox(box)
{

}

bool BoundFilter::include(const osmium::Way& way) 
{
  for(const auto& node: way.nodes()) {
    if(mBox.contains(node.location())) {
      return true;
    }
  }
  return false;
}

S2CellFilter::S2CellFilter(uint64_t id) 
{
  mS2Cell = make_unique<S2Cell>(S2CellId(id));
}

bool S2CellFilter::include(const osmium::Way& way) 
{
  for(const auto& node: way.nodes()) {
    if(mS2Cell->Contains(S2Point(S2LatLng::FromDegrees(node.location().lat(), node.location().lon())))) {
      return true;
    }
  }
  return false;
}

}