#ifndef MAPHANDLER_H
#define MAPHANDLER_H

#include <osmium/handler.hpp>
#include <assert.h>
#include <iostream>
#include <vector>

#include <osmium/index/node_locations_map.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

#include "main.h"
#include "osmsplitconfig.h"
#include "png.hpp"

using location_handler_type = osmium::handler::NodeLocationsForWays<NodeLocatorMap>;

namespace EngineBlock {

template<typename T, unsigned int D>
struct MapSplit {
public:

  struct Rect;
  using PairRect = std::pair<Rect,Rect>;

  struct Rect {
    T x, y, xlen, ylen;

    PairRect split(int midPoint, bool lon) {

      PairRect result = PairRect(*this, *this);

      if(lon) {
        result.first.xlen = midPoint;
        result.second.x = this->x + midPoint;
        result.second.xlen = xlen - midPoint;
      }
      else {
        result.first.ylen = midPoint;
        result.second.y = this->y + midPoint;
        result.second.ylen = ylen - midPoint;
      }
      return result;
    }
  };

  MapSplit() 
  : mMaxCell(0) 
  {
    mData = std::vector<T>(D * D, 0);
  }

  void incr(uint32_t x, uint32_t y) {
    
    T& v = index(x,y);
    v++;
    
    if(v > mMaxCell) { 
      mMaxCell = v;
    }
  }

  T& index(uint32_t x, uint32_t y) {
    
    assert(x <D && y < D);
    
    return mData[x + (y*D)];

  }

  PairRect split(Rect rect, bool lon, uint32_t& midPoint) {

    std::vector<T> list(lon ? rect.xlen : rect.ylen);

    int total = 0;
    for(uint32_t ypos=rect.y; ypos<rect.y+rect.ylen; ypos++) {
      for(uint32_t xpos=rect.x; xpos<rect.x+rect.xlen; xpos++) {

        T& v = index(xpos,ypos);  
        total += v;
        auto pos = lon ? (xpos - rect.x) : (ypos - rect.y);
        assert(pos < list.size());
        list[pos] += v;
      }
    }
    midPoint = 0;
    T count = list[midPoint];
    while(count < total / 2) {
      midPoint++;
      assert(midPoint < list.size());
      count += list[midPoint];
    }
    return rect.split(midPoint, lon);
  }

  const T& max() {
    return mMaxCell;
  }

protected:

  uint32_t mMaxCell;
  std::vector<T> mData;
};

using DblPair = std::tuple<double, double>;

template<typename T, unsigned int D>
class MapHandler : public location_handler_type
{
public:
  MapHandler(OSMSplitConfigPtr config, uint32_t sampleRate, NodeLocatorMap& store) 
  : location_handler_type(store),
    mSampleRate(sampleRate),
    mWayCount(0),
    mConfig(config)
  {
    const osmium::Box& extents = mConfig->getBox();

    mLocMin = DblPair(
      extents.bottom_left().lon(), 
      extents.bottom_left().lat()
    );

    mLocIncr = DblPair(
      (extents.top_right().lon() - extents.bottom_left().lon()) / (D),
      (extents.top_right().lat() - extents.bottom_left().lat()) / (D) 
    );


  }

  void node(const osmium::Node& node) {

    location_handler_type::node(node);

    if(mSampleCount++ >= mSampleRate) {

      const osmium::Location& loc = node.location();

      int x = (loc.lon() - std::get<0>(mLocMin)) / std::get<0>(mLocIncr);
      int y = (loc.lat() - std::get<1>(mLocMin)) / std::get<1>(mLocIncr);

      if(x > 0 && x < D && y > 0 && y < D) {
        mMap.incr(x,y);
      }

      mSampleCount = 0;
    }
  }

  void way(osmium::Way& way) {
    location_handler_type::way(way);

    mWayCount++;
  }

  void printMap() {

    mImage = png::image<png::rgb_pixel>(D, D);

    for (png::uint_32 y = 0; y < mImage.get_height(); ++y)
    {
        for (png::uint_32 x = 0; x < mImage.get_width(); ++x)
        { 
            T v = 255 * mMap.index(x,y) / mMap.max();
            mImage.set_pixel(x,y, png::rgb_pixel(v,v,v));
        }
    }

  }

  void finish(const std::string& imageName, uint32_t levels) {
    
    printMap();

    typename MapSplit<T,D>::Rect rect = {0, 0, D, D};

    split(levels, rect, mConfig);

    mImage.write(imageName);
  }

  void printSplit(uint32_t start, uint32_t len, uint32_t midPoint, bool lon) {

    for(uint32_t i=start; i<len+start; i++) {

      mImage.set_pixel(lon?midPoint:i,lon?i:midPoint, png::rgb_pixel(233,34,12));
    }
  }

  double midPointReal(uint32_t midPoint, bool lon) {

    double proportion = (double)midPoint / D;
    osmium::Box extents = mConfig->getBox();
    if(lon) {

      double width = extents.top_right().lon() - extents.bottom_left().lon();
      return width * proportion + extents.bottom_left().lon();
    }
    else {

      double height = extents.top_right().lat() - extents.bottom_left().lat();
      return height * proportion + extents.bottom_left().lat();
    }
  }
  
  uint64_t numWays() {
    return mWayCount;
  }
  void split(int levels, typename MapSplit<T,D>::Rect rect, OSMSplitConfigPtr config) {

    // for(int i=0; i<6-levels; i++) {
    //   std::cout << "\t";
    // }
    // std::cout << "Rect " << rect.x << " " << rect.y << " " << rect.xlen << " " << rect.ylen << std::endl;

    bool lat = config->sortByLat();
    if(levels--) {

      uint32_t midPoint;
      auto rectSplits = mMap.split(rect, !lat, midPoint);

      double midPointR;
      if(lat) {
        midPointR = midPointReal(midPoint+rect.y, !lat);
        printSplit(rect.x, rect.xlen, midPoint+rect.y, !lat);
      }
      else {
        midPointR = midPointReal(midPoint+rect.x, !lat);
        printSplit(rect.y, rect.ylen, midPoint+rect.x, !lat);
      }
      auto configPair = config->split(midPointR);
      
      split(levels, rectSplits.first, configPair.first);
      split(levels, rectSplits.second, configPair.second);
    }
  }

protected:
  DblPair             mLocMin;
  DblPair             mLocIncr;
  osmium::Box         mExtents;
  MapSplit<T,D>       mMap;
  uint32_t            mSampleRate;
  uint32_t            mSampleCount;
  uint64_t            mWayCount;
  OSMSplitConfigPtr   mConfig;
  png::image <png::rgb_pixel> mImage;
};


}

#endif
