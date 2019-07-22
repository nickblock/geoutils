#ifndef COMMON_USING_H
#define COMMON_USING_H

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <utility>
#include <thread>
#include <memory>
#include <mutex>
#include <map>

namespace EngineBlock {

using glm::vec3;
using glm::vec4;
using glm::mat4;

using std::vector;
using std::string;
using std::stringstream;
using std::cout;
using std::endl;
using std::numeric_limits;
using std::shared_ptr;
using std::make_shared;
using std::thread;  
using std::tuple;
using std::tie;
using std::get;
using std::make_tuple;

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

}

#endif