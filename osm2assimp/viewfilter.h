#pragma once

#include <osmium/osm/way.hpp>
#include <vector>
#include <memory>

class S2Cell;

namespace GeoUtils {

class ViewFilter;

/// <summary>
/// A container that defines the filter of the data.
/// That may include the extents of tbe bounds we are taking, any type filter, etc.
/// In order for an osmium::way to be included in the view it must pass all filters
/// </summary
using ViewFilterList = std::vector<ViewFilter&>;

/// A class which decides whehter a given osmium::way should be included in our view
class ViewFilter {
public:
  virtual bool include(const osmium::Way& way) = 0;
};

/// A filter of types as defined in OSMFeature
class TypeFilter : public ViewFilter
{
public:
  TypeFilter(int filter);

  virtual bool include(const osmium::Way& way);

private:
  int mFilter;
};

class BoundFilter : public ViewFilter
{
public:
  BoundFilter(const osmium::Box& box);

  virtual bool include(const osmium::Way& way);

private:
  const osmium::Box& mBox;
};


class S2CellFilter : public ViewFilter
{
public:
  S2CellFilter(uint64_t id);

  virtual bool include(const osmium::Way& way);

private:
  std::unique_ptr<S2Cell> mS2Cell;
};

}