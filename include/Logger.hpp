#pragma once

#include <algorithm>
#include <stdint.h>
#include <string.h>
#include <memory>
#include <limits>
#include <iostream>
#include <stdarg.h>
#include <algorithm>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "Utils.hpp"
#include "LogFile.hpp"
#include "SingleCache.hpp"
#include "TimeUtil.hpp"

namespace netio {

#define LOG_BUF_SIZE  (SIZE_K(2))

typedef enum {
  LEVEL_FATAL = 0,
  LEVEL_ERROR,
  LEVEL_WARRING,
  LEVEL_INFO,
  LEVEL_DEBUG,
  LEVEL_VERBOSE,
  LEVEL_MAX
} LOG_LEVEL;

#define LEVEL_VALUE_FATAL   (1 << LEVEL_FATAL)
#define LEVEL_VALUE_ERROR   (1 << LEVEL_ERROR)
#define LEVEL_VALUE_WARRING (1 << LEVEL_WARRING)
#define LEVEL_VALUE_INFO    (1 << LEVEL_INFO)
#define LEVEL_VALUE_DEBUG   (1 << LEVEL_DEBUG)
#define LEVEL_VALUE_VERBOSE (1 << LEVEL_VERBOSE)

typedef struct  {
  char _desc[5];
  size_t _len;
} LogLevelInfo;

extern LogLevelInfo g_loglevel_infos[];

template <bool CON>
struct CacheAppend {
  static void append(SingleCache<>& _cache, const char* str, size_t len) {
    _cache.append(str, len);
  }
};

template <>
struct CacheAppend<true> {
  static void append(SingleCache<>& _cache, const char* str, size_t len) {
    std::cout << str;
    _cache.append(str, len);
  }
};

constexpr int getLogTimezoneWestSec() {
  // struct timezone tz;
  // struct timeval tv;
  // gettimeofday(&tv, &tz);
  
  // return tz.tz_minuteswest * 60;
  return 8 * 60 * 60;
}

static constexpr int LOG_FMT_TIME_LEN =  strlen("00/00 00:00:00 ");
static constexpr int LOG_TZ_SEC_WEST = getLogTimezoneWestSec();
/**
 * tempalte CON : specify that if will print log on console. Default closed, it
 * will open usually when unit testing.
 */
template <bool CON = false>
class Logger {
  static const int MAX_NUM_LEN = 32;
#define LOGGER_ASSERT_STR "not enough for store numeric"
  static_assert(MAX_NUM_LEN - 10 > std::numeric_limits<double>::digits10, LOGGER_ASSERT_STR);
  static_assert(MAX_NUM_LEN - 10 > std::numeric_limits<long double>::digits10, LOGGER_ASSERT_STR);
  static_assert(MAX_NUM_LEN - 10 > std::numeric_limits<long>::digits10, LOGGER_ASSERT_STR);
  static_assert(MAX_NUM_LEN - 10 > std::numeric_limits<long long>::digits10, LOGGER_ASSERT_STR);
public:
  typedef Logger<CON> self;
  template <typename T1, typename T2>
  Logger(T1&& basePath, T2&& prefix) : _cache(basePath, prefix) {
    setLogLevel(LEVEL_VERBOSE);
  }

  void printLogLn(int logLevel, const char* tag, const char * format, ... ) {
    if(LIKELY(logLevel < LEVEL_MAX)) {
      char buf[LOG_BUF_SIZE] = {0};
      int ret;
      int bufIdx = 0;
      
      // if log level that dosn't need to print.
      if(0 == (_levelMask & (1 << logLevel))) {
        return;
      }

      int tmpLen = 0;
      
      // append time at the begining of log
      tmpLen = appendLogTime(buf+bufIdx);
      bufIdx += tmpLen;
      
      // write level info first
      tmpLen = g_loglevel_infos[logLevel]._len;
      memcpy(buf+bufIdx, g_loglevel_infos[logLevel]._desc, tmpLen);
      bufIdx += tmpLen;

      if(tag != nullptr) {
        // append tag info
        tmpLen = strlen(tag);
        memcpy(buf+bufIdx, tag, tmpLen);
        bufIdx += tmpLen;

        // append tab
        buf[bufIdx] = ':';
        buf[bufIdx+1] = '\t';
        bufIdx += 2;        
      }

      va_list args;
      va_start (args, format);
      ret = vsnprintf(buf + bufIdx, sizeof(buf) - bufIdx, format, args);
      va_end (args);
    
      if(LIKELY(ret > 0)) {
        bufIdx += ret;
        // we will append '\n' auto
        bufIdx = min(bufIdx, static_cast<int>(sizeof(buf) - 1));
        buf[bufIdx] = '\n';
        append(buf, bufIdx + 1);
      }
    } else {
      fprintf(stderr, "ERR, write invalid log level : %d", logLevel);
    }
  }

  void setLogLevel(LOG_LEVEL level) {
    ASSERT(level < LEVEL_MAX);
    // mark bits that with level higher than input param.
    _levelMask = (1 << (level + 1)) - 1;
  }
 
 private:
  
  template <typename T>
  void appendNumeric(T value) {
    char temp[MAX_NUM_LEN] = {0};
    size_t len = stringFromNumeric(temp, value);
    append(temp, len);
  }

  void append(const char* str) {
    append(str, strlen(str));
  }

  /** 
   * real append 
   */
  void append(const char* str, size_t len) {
    CacheAppend<CON>::append(_cache, str, len);
  }

  
  template <typename T>
  size_t stringFromNumeric(char buf[], T value) {
    static const char digits[] = "9876543210123456789";
    static const char* zero = digits + 9;
    static_assert(sizeof(digits) == 20, "digits string size is not 20");

    T i = value;
    char* p = buf;

    do
    {
      int lsd = static_cast<int>(i % 10);
      i /= 10;
      *p++ = zero[lsd];
    } while (i != 0);
  
    if (value < 0)
    {
      *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
  }

  size_t hexStringFromNumeric(char buf[], uintptr_t value)
  {
    static const char digitsHex[] = "0123456789ABCDEF";
    static_assert(sizeof(digitsHex) == 17, "digits size is not 17 !!");

    uintptr_t i = value;
    char* p = buf;

    do
    {
      int lsd = static_cast<int>(i % 16);
      i /= 16;
      *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
  }

  /** 
   * append "00/00 00:00:00 " time info at the address, we assume that
   * addr has enough buffer for time info.
   * 
   * @param addr 
   * 
   * @return time info length
   */
  const int appendLogTime(char* addr) const {
    // struct timeval tv;
    // struct timezone tz;
    struct tm timeinfo;

    time_t ts;

//    gettimeofday(&tv, &tz);

    time(&ts);
    ts = ts + LOG_TZ_SEC_WEST;
    gmtime_r(&ts, &timeinfo);
    sprintf(addr, "%02d/%02d %02d:%02d:%02d ", timeinfo.tm_mon, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return LOG_FMT_TIME_LEN;
  }
  
  SingleCache<> _cache;
  uint32_t _levelMask;
};

extern Logger<true> g_inner_logger;
extern const char* g_common_tag;
extern const char* LOG_NETIO_TAG;
extern const char* g_syserr_tag;

#define LOGF(...) g_inner_logger.printLogLn(LEVEL_FATAL, __VA_ARGS__)
#define LOGE(...) g_inner_logger.printLogLn(LEVEL_ERROR, __VA_ARGS__)
#define LOGW(...) g_inner_logger.printLogLn(LEVEL_WARRING, __VA_ARGS__)
#define LOGI(...) g_inner_logger.printLogLn(LEVEL_INFO, __VA_ARGS__)
#define LOGD(...) g_inner_logger.printLogLn(LEVEL_DEBUG, __VA_ARGS__)
#define LOGV(...) g_inner_logger.printLogLn(LEVEL_VERBOSE, __VA_ARGS__)

#define FOGF(...) g_inner_logger.printLogLn(LEVEL_FATAL, __FILE__, __VA_ARGS__)
#define FOGE(...) g_inner_logger.printLogLn(LEVEL_ERROR, __FILE__, __VA_ARGS__)
#define FOGW(...) g_inner_logger.printLogLn(LEVEL_WARRING, __FILE__, __VA_ARGS__)
#define FOGI(...) g_inner_logger.printLogLn(LEVEL_INFO, __FILE__, __VA_ARGS__)
#define FOGD(...) g_inner_logger.printLogLn(LEVEL_DEBUG, __FILE__, __VA_ARGS__)
#define FOGV(...) g_inner_logger.printLogLn(LEVEL_VERBOSE, __FILE__, __VA_ARGS__)

#define COGF(...) g_inner_logger.printLogLn(LEVEL_FATAL, g_common_tag, __VA_ARGS__)
#define COGE(...) g_inner_logger.printLogLn(LEVEL_ERROR, g_common_tag, __VA_ARGS__)
#define COGW(...) g_inner_logger.printLogLn(LEVEL_WARRING, g_common_tag, __VA_ARGS__)
#define COGI(...) g_inner_logger.printLogLn(LEVEL_INFO, g_common_tag, __VA_ARGS__)
#define COGD(...) g_inner_logger.printLogLn(LEVEL_DEBUG, g_common_tag, __VA_ARGS__)
#define COGV(...) g_inner_logger.printLogLn(LEVEL_VERBOSE, g_common_tag, __VA_ARGS__)

#define COGFUNC() g_inner_logger.printLogLn(LEVEL_VERBOSE, g_common_tag, __func__)
#define LOGSYSERR() g_inner_logger.printLogLn(LEVEL_ERROR, g_syserr_tag, "system error, (%d) message=%s", errno, strerror(errno))

}




