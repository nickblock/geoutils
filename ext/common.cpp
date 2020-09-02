#include "common.h"
#include <iostream>
#include <assert.h>
#include <iomanip>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common_using.h"

using std::fixed;
using std::setprecision;

namespace EngineBlock
{

  //collision utils
  rapidjson::Value toJson(const glm::vec4 &v, rapidjson::Document::AllocatorType &alloc)
  {
    rapidjson::Value json(rapidjson::kArrayType);

    json.PushBack(v.x, alloc);
    json.PushBack(v.y, alloc);
    json.PushBack(v.z, alloc);
    json.PushBack(v.w, alloc);

    return json;
  }

  rapidjson::Value toJson(const glm::vec3 &v, rapidjson::Document::AllocatorType &alloc)
  {
    rapidjson::Value json(rapidjson::kArrayType);

    json.PushBack(v.x, alloc);
    json.PushBack(v.y, alloc);
    json.PushBack(v.z, alloc);

    return json;
  }
  rapidjson::Value toJson(const glm::vec2 &v, rapidjson::Document::AllocatorType &alloc)
  {
    rapidjson::Value json(rapidjson::kArrayType);

    json.PushBack(v.x, alloc);
    json.PushBack(v.y, alloc);

    return json;
  }
  rapidjson::Value toJson(const glm::ivec2 &v, rapidjson::Document::AllocatorType &alloc)
  {
    rapidjson::Value json(rapidjson::kArrayType);

    json.PushBack(v.x, alloc);
    json.PushBack(v.y, alloc);

    return json;
  }

  const rapidjson::Value &subDotValue(const rapidjson::Value &json, const string &mem, bool &success)
  {

    success = true;

    const rapidjson::Value *retValue = &json;
    auto begin = mem.cbegin();
    auto cur = begin;
    auto end = mem.cend();

    while (cur != end)
    {

      if (*(++cur) == '.')
      {
        string member(begin, cur);

        if (retValue->HasMember(member.c_str()))
        {

          retValue = &(*retValue)[member.c_str()];
        }
        else
        {
          success = false;
          return *retValue;
        }

        begin = ++cur;
      }
    }

    string member(begin, cur);

    if (retValue->HasMember(member.c_str()))
    {

      retValue = &(*retValue)[member.c_str()];
    }
    else
    {
      success = false;
    }

    return *retValue;
  }

  glm::vec3 fromJson(const rapidjson::Value &json)
  {

    return glm::vec3(json[0].GetDouble(), json[1].GetDouble(), json[2].GetDouble());
  }

  glm::ivec2 ivec2fromJson(const rapidjson::Value &json)
  {
    return glm::ivec2(json[0].GetInt(), json[1].GetInt());
  }

  template <>
  float fromJson(const rapidjson::Value &value, const std::string &member, float defaultVal)
  {
    bool success = false;
    auto &subVal = subDotValue(value, member, success);
    if (success)
    {
      return subVal.GetFloat();
    }
    else
    {
      return defaultVal;
    }
  }

  template <>
  int fromJson(const rapidjson::Value &value, const std::string &member, int defaultVal)
  {
    bool success = false;
    auto &subVal = subDotValue(value, member, success);
    if (success)
    {
      return subVal.GetInt();
    }
    else
    {
      return defaultVal;
    }
  }

  template <>
  bool fromJson(const rapidjson::Value &value, const std::string &member, bool defaultVal)
  {
    bool success = false;
    auto &subVal = subDotValue(value, member, success);
    if (success)
    {
      return subVal.GetBool();
    }
    else
    {
      return defaultVal;
    }
  }

  template <>
  string fromJson(const rapidjson::Value &value, const std::string &member, string defaultVal)
  {
    bool success = false;
    auto &subVal = subDotValue(value, member, success);
    if (success)
    {
      return subVal.GetString();
    }
    else
    {
      return defaultVal;
    }
  }

  template <>
  glm::vec3 fromJson(const rapidjson::Value &value, const std::string &member, glm::vec3 defaultVal)
  {
    bool success = false;
    auto &subVal = subDotValue(value, member, success);
    if (success)
    {
      return fromJson(subVal);
    }
    else
    {
      return defaultVal;
    }
  }

  template <>
  StringList fromJson(const rapidjson::Value &value, const std::string &member, StringList defaultVal)
  {
    bool success = false;
    auto &subVal = subDotValue(value, member, success);
    if (success)
    {
      StringList ret;
      auto &jStringList = subVal;
      if (jStringList.IsArray())
      {
        for (int i = 0; i < jStringList.Size(); i++)
        {
          ret.push_back(jStringList[i].GetString());
        }
      }
      else
      {
        ret.push_back(jStringList.GetString());
      }
      return ret;
    }
    else
    {
      return defaultVal;
    }
  }

  TimePoint millisNow()
  {
    return Clock::now();
  }

  void printTransform(const glm::mat4 &m)
  {
    cout << fixed << setprecision(2);

    for (int r = 0; r < 4; r++)
    {
      for (int c = 0; c < 4; c++)
      {
        cout << m[r][c] << " ";
      }
      cout << endl;
    }
  }

  glm::vec3 posFromMat4(const glm::mat4 &mat)
  {
    return glm::make_vec3(glm::value_ptr(mat) + 12);
  }

  void safe_printf(const char *s)
  {
    while (*s)
    {
      if (*s == '%')
      {
        if (*(s + 1) == '%')
        {
          ++s;
        }
        else
        {
          assert(0);
        }
      }
      std::cout << *s++;
    }
  }

  void printVec3(const glm::vec3 &v)
  {
    std::cout << v.x << " " << v.y << " " << v.z << std::endl;
  }

  bool match(const std::string &needle, const std::string &haystack)
  {
    return match(needle.c_str(), haystack.c_str());
  }

  bool match(char const *needle, char const *haystack)
  {
    for (; *needle != '\0'; ++needle)
    {
      switch (*needle)
      {
      case '?':
        if (*haystack == '\0')
          return false;
        ++haystack;
        break;
      case '*':
      {
        if (needle[1] == '\0')
          return true;
        size_t max = strlen(haystack);
        for (size_t i = 0; i < max; i++)
          if (match(needle + 1, haystack + i))
            return true;
        return false;
      }
      default:
        if (*haystack != *needle)
          return false;
        ++haystack;
      }
    }
    return *haystack == '\0';
  }

  void replaceAll(std::string &str, const std::string &from, const std::string &to)
  {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  }

} // namespace EngineBlock