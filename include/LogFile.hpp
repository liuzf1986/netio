#pragma once

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <mutex>
#include <memory>
#include <string>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "FileUtil.hpp"
#include "Utils.hpp"

using namespace std;

namespace netio
{
class DailyLogFile {
  typedef shared_ptr<vector<int8_t>> SpCache;
 public:
  template <typename T1, typename T2>
  DailyLogFile(T1&& basePath, T2&& prefix) :
      _ready(false),
      _logging(true),
      _basePath(std::forward<T1>(basePath)),
      _prefix(std::forward<T2>(prefix)),
      _file(nullptr),
      _mutex(),
      _cond(),
      _thread(&DailyLogFile::looper, this)
  {
    if(_basePath.length() > 0) {
      ASSERT(FileUtil::dirWrittable(_basePath));
      
      // Fix base file path with '/'
      if((_basePath.length() - 1) != _basePath.find_last_of('/')) {
        _basePath += "/";
      } 
    } else {
      _basePath = "./";
    }

    createNewFile();
  };

  ~DailyLogFile();

  void flush(SpCache& spVec);

 private:
  // process SpCache
  void processNoLock();


      
      
     


  
  
  // ready flag
  bool _ready;
  //  bool _logging;
  atomic<bool> _logging;
  
  // for create file.
  string _basePath;
  string _prefix;
  FILE* _file;
  void createNewFile();
  void recreateNewFile();  // thread safe
  void closeFile();
  char* getFileName() const;

  // cache queue to store log peice
  queue<SpCache> _cacheQueue;

  // thread safe.
  mutable mutex _mutex;
  condition_variable _cond;

  // thread looper.
  thread _thread;
  void looper();
};
}

