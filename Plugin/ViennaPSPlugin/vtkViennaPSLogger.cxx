#include "vtkViennaPSLogger.h"
#include <vtkObjectBase.h>
#include <vtkSetGet.h>

namespace ViennaPSLogger {

void Logger::LogToVTK(vtkObject* obj, LogLevel level, const std::string& message) {
  std::string prefix;
  
  switch(level) {
    case LogLevel::DEBUG:
      prefix = "[DEBUG] ";
      break;
    case LogLevel::INFO:
      prefix = "[INFO] ";
      break;
    case LogLevel::WARNING:
      prefix = "[WARNING] ";
      break;
    case LogLevel::ERROR:
      prefix = "[ERROR] ";
      break;
    default:
      prefix = "";
  }

  std::string fullMessage = prefix + message;

  if (level >= LogLevel::ERROR) {
    vtkOutputWindowDisplayErrorText(fullMessage.c_str());
  } else if (level >= LogLevel::WARNING) {
    vtkOutputWindowDisplayWarningText(fullMessage.c_str());
  } else {
    vtkOutputWindowDisplayDebugText(fullMessage.c_str());
  }
}

} 