#ifndef __EB_COMMON_H__
#define __EB_COMMON_H__

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <rapidjson/document.h>
#include "tinyformat.h"

#include <vector>
#include <iostream>
#include <assert.h>
#include <chrono>

#define USE_VIEWER 1
#define USE_EXCEPTIONS 1

namespace EngineBlock {

  using IntPair = std::pair<int, int>;
  using PointList = std::vector<glm::vec3>;
  using DataBuffer = std::vector<unsigned char>;
  using FloatBuffer = std::vector<float>;
  using StringList = std::vector<std::string>;
  using TimePoint = std::chrono::high_resolution_clock::time_point;
  using Duration = std::chrono::milliseconds;

  typedef unsigned char uint8;
  typedef unsigned short uint16;
  typedef unsigned int uint32;

  const rapidjson::Value& subDotValue(const rapidjson::Value& json, const std::string& mem, bool& success);

  template<typename T>
  T fromJson(const rapidjson::Value& value, const std::string& member, T defaultVal);

  rapidjson::Value toJson(const glm::vec4&, rapidjson::Document::AllocatorType&);
  rapidjson::Value toJson(const glm::vec3&, rapidjson::Document::AllocatorType&);
  rapidjson::Value toJson(const glm::vec2&, rapidjson::Document::AllocatorType&);
  rapidjson::Value toJson(const glm::ivec2&, rapidjson::Document::AllocatorType&);
  glm::vec3 fromJson(const rapidjson::Value&);
  glm::ivec2 ivec2fromJson(const rapidjson::Value&);

  void printVec3(const glm::vec3&); 
  
  TimePoint millisNow();

  bool match(const std::string& needle, const std::string& haystack);
  bool match(char const *needle, char const *haystack);

  void printTransform(const glm::mat4&);
  glm::vec3 posFromMat4(const glm::mat4&);

  void replaceAll(std::string& str, const std::string& from, const std::string& to);
}

#if DEBUG
#define ASSERTM(condition, m, ...) \
{ \
  if(!(condition)) { \
    tfm::printf(m, ##__VA_ARGS__); \
    printf("\n"); \
    assert(0); \
  }   \
} 
#else 
#define ASSERTM ; 
#endif 


#endif //__EB_COMMON_H__