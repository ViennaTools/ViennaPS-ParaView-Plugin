#ifndef vtkViennaPSLogger_h
#define vtkViennaPSLogger_h

#include <iostream>
#include <sstream>
#include <mutex>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
#else
    #include <execinfo.h>
    #include <unistd.h>
#endif

class vtkObject;

namespace ViennaPSLogger {

enum class LogLevel {
  DEBUG = 0,
  INFO = 1,
  WARNING = 2,
  ERROR = 3,
  NONE = 4
};

class Logger {
public:
  static Logger& Instance() {
    static Logger instance;
    return instance;
  }

  void SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentLevel_ = level;
  }

  LogLevel GetLevel() const {
    return currentLevel_;
  }

  template<typename... Args>
  void Log(vtkObject* obj, LogLevel level, Args&&... args) {
    if (level < currentLevel_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream oss;
    ((oss << std::forward<Args>(args)), ...);
    std::string message = oss.str();

    if (obj != nullptr) {
      LogToVTK(obj, level, message);
    } else {
      LogToConsole(level, message);
    }
  }

  template<typename... Args>
  void LogWithStackTrace(vtkObject* obj, LogLevel level, Args&&... args) {
    if (level < currentLevel_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;
    ((oss << std::forward<Args>(args)), ...);
    std::string message = oss.str();

    std::string stackTrace = GetStackTrace();
    std::string fullMessage = message + "\n" + stackTrace;

    if (obj != nullptr) {
      LogToVTK(obj, level, fullMessage);
    } else {
      LogToConsole(level, fullMessage);
    }
  }

  std::string GetStackTrace() {
    std::ostringstream oss;
    
#ifdef _WIN32
    void* stack[64];
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    USHORT frames = CaptureStackBackTrace(0, 64, stack, NULL);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256, 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    oss << "Callstack (Windows):" << std::endl;
    for (USHORT i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        oss << i << ": " << symbol->Name << " - 0x"
            << std::hex << symbol->Address << std::dec << std::endl;
    }
    free(symbol);

#else
    const int maxFrames = 64;
    void* frames[maxFrames];
    int frameCount = backtrace(frames, maxFrames);
    char** symbols = backtrace_symbols(frames, frameCount);

    oss << "Callstack (Linux):" << std::endl;
    for (int i = 0; i < frameCount; i++) {
        oss << symbols[i] << std::endl;
    }
    free(symbols);
#endif
    
    return oss.str();
  }

private:
  Logger() : currentLevel_(LogLevel::DEBUG) {}
  
  void LogToConsole(LogLevel level, const std::string& message) {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::cout << std::put_time(&tm, "%H:%M:%S") << " ";
    
    std::cout << "[" << LevelToString(level) << "] ";
    
    std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
    out << message << std::endl;
  }

  void LogToVTK(vtkObject* obj, LogLevel level, const std::string& message);

  const char* LevelToString(LogLevel level) {
    switch(level) {
      case LogLevel::DEBUG:   return "DEBUG";
      case LogLevel::INFO:    return "INFO ";
      case LogLevel::WARNING: return "WARN ";
      case LogLevel::ERROR:   return "ERROR";
      default:                return "?????";
    }
  }

  LogLevel currentLevel_;
  std::mutex mutex_;
};

#define VPSLOG_DEBUG(obj, ...)   ViennaPSLogger::Logger::Instance().Log(obj, ViennaPSLogger::LogLevel::DEBUG, __VA_ARGS__)
#define VPSLOG_INFO(obj, ...)    ViennaPSLogger::Logger::Instance().Log(obj, ViennaPSLogger::LogLevel::INFO, __VA_ARGS__)
#define VPSLOG_WARNING(obj, ...) ViennaPSLogger::Logger::Instance().Log(obj, ViennaPSLogger::LogLevel::WARNING, __VA_ARGS__)
#define VPSLOG_ERROR(obj, ...)   ViennaPSLogger::Logger::Instance().Log(obj, ViennaPSLogger::LogLevel::ERROR, __VA_ARGS__)

#define VPSLOG_IF(obj, condition, level, ...) \
  if (condition) ViennaPSLogger::Logger::Instance().Log(obj, level, __VA_ARGS__)

#define VPSLOG_SET_LEVEL(level) ViennaPSLogger::Logger::Instance().SetLevel(level)

// Print stack trace only, no log level
#define VPSLOG_PRINT_STACK() \
  std::cout << ViennaPSLogger::Logger::Instance().GetStackTrace() << std::endl

}

#endif
