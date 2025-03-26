#pragma once

#include <iostream>

namespace utils {

///////////////LOGGING FUNCTIONS///////////////////////
//--log type
#define LOG_DEBUG 0                // 0
#define LOG_VERBOSE 1              // 1
#define LOG_INFO 2                 // 2
#define LOG_NOTICE 3               // 3
#define LOG_WARN 4                 // 4
#define LOG_ERROR 5                // 5
#define LOG_FATAL 6                // 6
#define LOG_OK 7                   // 7
#define GLOBAL_LOG_LEVEL LOG_VERBOSE // change verbose level here

}  // namespace utils

// only for c++17...
// namespace utils::log_level {

// ///////////////LOGGING FUNCTIONS///////////////////////
// //--log type
// inline constexpr int LOG_DEBUG = 0;                // 0
// inline constexpr int LOG_VERBOSE = 1;              // 1
// inline constexpr int LOG_INFO = 2;                 // 2
// inline constexpr int LOG_NOTICE = 3;               // 3
// inline constexpr int LOG_WARN = 4;                 // 4
// inline constexpr int LOG_ERROR = 5;                // 5
// inline constexpr int LOG_FATAL = 6;                // 6
// inline constexpr int LOG_OK = 7;                   // 7
// inline int GLOBAL_LOG_LEVEL = LOG_INFO;  // change verbose level in Setting.h

// }  // namespace utils::log_level

// using namespace utils::log_level;