////////////////////////////////////////////////////////////////////////////////
/// @brief logging macros and functions
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2014 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2014, ArangoDB GmbH, Cologne, Germany
/// @author Copyright 2011-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_BASICS_C_LOGGING_H
#define ARANGODB_BASICS_C_LOGGING_H 1

#include "Basics/Common.h"

#include "Basics/vector.h"

// -----------------------------------------------------------------------------
// --SECTION--                                                           LOGGING
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                      public types
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief log levels
////////////////////////////////////////////////////////////////////////////////

typedef enum {
  TRI_LOG_LEVEL_FATAL   = 1,
  TRI_LOG_LEVEL_ERROR   = 2,
  TRI_LOG_LEVEL_WARNING = 3,
  TRI_LOG_LEVEL_INFO    = 4,
  TRI_LOG_LEVEL_DEBUG   = 5,
  TRI_LOG_LEVEL_TRACE   = 6
}
TRI_log_level_e;

////////////////////////////////////////////////////////////////////////////////
/// @brief log severities
////////////////////////////////////////////////////////////////////////////////

typedef enum {
  TRI_LOG_SEVERITY_EXCEPTION   = 1,
  TRI_LOG_SEVERITY_TECHNICAL   = 2,
  TRI_LOG_SEVERITY_FUNCTIONAL  = 3,
  TRI_LOG_SEVERITY_DEVELOPMENT = 4,
  TRI_LOG_SEVERITY_USAGE       = 5,
  TRI_LOG_SEVERITY_HUMAN       = 6,
  TRI_LOG_SEVERITY_UNKNOWN     = 7
}
TRI_log_severity_e;

////////////////////////////////////////////////////////////////////////////////
/// @brief log categories
////////////////////////////////////////////////////////////////////////////////

typedef enum {

  // exceptions
  TRI_LOG_CATEGORY_FATAL             = 1000,
  TRI_LOG_CATEGORY_ERROR             = 1001,
  TRI_LOG_CATEGORY_WARNING           = 1002,

  // technical
  TRI_LOG_CATEGORY_HEARTBEAT         = 2000,
  TRI_LOG_CATEGORY_REQUEST_IN_END    = 2001,
  TRI_LOG_CATEGORY_REQUEST_IN_START  = 2002,
  TRI_LOG_CATEGORY_REQUEST_OUT_END   = 2003,
  TRI_LOG_CATEGORY_REQUEST_OUT_START = 2004,

  // development
  TRI_LOG_CATEGORY_FUNCTION_IN_END   = 4000,
  TRI_LOG_CATEGORY_FUNCTION_IN_START = 4001,
  TRI_LOG_CATEGORY_HEARTPULSE        = 4002,
  TRI_LOG_CATEGORY_LOOP              = 4003,
  TRI_LOG_CATEGORY_MODULE_IN_END     = 4004,
  TRI_LOG_CATEGORY_MODULE_IN_START   = 4005,
  TRI_LOG_CATEGORY_STEP              = 4006
}
TRI_log_category_e;

////////////////////////////////////////////////////////////////////////////////
/// @brief buffer type
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_log_buffer_s {
  uint64_t _lid;
  TRI_log_level_e _level;
  time_t _timestamp;
  char* _text;
}
TRI_log_buffer_t;

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief gets the log level
////////////////////////////////////////////////////////////////////////////////

char const* TRI_LogLevelLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the log level
////////////////////////////////////////////////////////////////////////////////

void TRI_SetLogLevelLogging (char const* level);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the log severity
////////////////////////////////////////////////////////////////////////////////

void TRI_SetLogSeverityLogging (char const* severities);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the output prefix
////////////////////////////////////////////////////////////////////////////////

void TRI_SetPrefixLogging (char const*);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the thread identifier visibility
////////////////////////////////////////////////////////////////////////////////

void TRI_SetThreadIdentifierLogging (bool);

////////////////////////////////////////////////////////////////////////////////
/// @brief use local time?
////////////////////////////////////////////////////////////////////////////////

void TRI_SetUseLocalTimeLogging (bool);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the line number visibility
////////////////////////////////////////////////////////////////////////////////

void TRI_SetLineNumberLogging (bool show);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the function names visibility
////////////////////////////////////////////////////////////////////////////////

void TRI_SetFunctionLogging (bool show);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the file to log for debug and trace
////////////////////////////////////////////////////////////////////////////////

void TRI_SetFileToLog (char const* file);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if usage logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsUsageLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if human logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsHumanLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if exception logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsExceptionLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if technical logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsTechnicalLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if functional logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsFunctionalLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if development logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsDevelopmentLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if fatal logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsFatalLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if error logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsErrorLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if warning logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsWarningLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if info logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsInfoLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if debug logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsDebugLogging (char const*);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if trace logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsTraceLogging (char const*);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if performance logging is enabled
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsPerformanceLogging ();

////////////////////////////////////////////////////////////////////////////////
/// @brief logs a new message
////////////////////////////////////////////////////////////////////////////////

void TRI_Log (char const*,
              char const*,
              int,
              TRI_log_level_e,
              TRI_log_severity_e,
              char const*,
              ...);

////////////////////////////////////////////////////////////////////////////////
/// @brief logs a new raw message
////////////////////////////////////////////////////////////////////////////////

void TRI_RawLog (TRI_log_level_e,
                 TRI_log_severity_e,
                 char const*,
                 size_t);

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the last log entries
////////////////////////////////////////////////////////////////////////////////

TRI_vector_t* TRI_BufferLogging (TRI_log_level_e, uint64_t pos, bool useUpto);

////////////////////////////////////////////////////////////////////////////////
/// @brief frees the log buffer
////////////////////////////////////////////////////////////////////////////////

void TRI_FreeBufferLogging (TRI_vector_t* buffer);

// -----------------------------------------------------------------------------
// --SECTION--                                                     public macros
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief macro that validates printf() style call arguments
/// the printf() call contained will never be executed but is just there to
/// enable compile-time error check. it will be optimised away after that
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_ENABLE_LOGGER

#define LOG_ARG_CHECK(...)                                                                                 \
  if (false) {                                                                                             \
    printf(__VA_ARGS__);                                                                                   \
  }                                                                                                        \

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs fatal errors
////////////////////////////////////////////////////////////////////////////////

void CLEANUP_LOGGING_AND_EXIT_ON_FATAL_ERROR (void);

#ifdef TRI_ENABLE_LOGGER

#define LOG_FATAL_AND_EXIT(...)                                         \
  do {                                                                  \
    LOG_ARG_CHECK(__VA_ARGS__);                                         \
    if (TRI_IsHumanLogging() && TRI_IsFatalLogging()) {                 \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__,                         \
              TRI_LOG_LEVEL_FATAL,                                      \
              TRI_LOG_SEVERITY_HUMAN,                                   \
              __VA_ARGS__);                                             \
      std::string bt;                                                   \
      TRI_GetBacktrace(bt);                                             \
      if (! bt.empty()) {                                               \
        TRI_Log(__FUNCTION__, __FILE__, __LINE__,                       \
                TRI_LOG_LEVEL_ERROR,                                    \
                TRI_LOG_SEVERITY_HUMAN,                                 \
                "%s", bt.c_str());                                      \
      }                                                                 \
    }                                                                   \
    CLEANUP_LOGGING_AND_EXIT_ON_FATAL_ERROR();                          \
  } while (0);                                                          \
  exit(EXIT_FAILURE)                                                    

#else

#define LOG_FATAL_AND_EXIT(...)                                         \
  do {                                                                  \
    CLEANUP_LOGGING_AND_EXIT_ON_FATAL_ERROR();                          \
  } while (0);                                                          \
  exit(EXIT_FAILURE)                                                    

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs errors
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_ENABLE_LOGGER

#define LOG_ERROR(...)                                                                                     \
  do {                                                                                                     \
    LOG_ARG_CHECK(__VA_ARGS__)                                                                             \
    if (TRI_IsHumanLogging() && TRI_IsErrorLogging()) {                                                    \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__, TRI_LOG_LEVEL_ERROR, TRI_LOG_SEVERITY_HUMAN, __VA_ARGS__); \
    }                                                                                                      \
  } while (0)

#else

#define LOG_ERROR(...) while (0)

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs warnings
////////////////////////////////////////////////////////////////////////////////

#undef LOG_WARNING

#ifdef TRI_ENABLE_LOGGER

#define LOG_WARNING(...)                                                                                     \
  do {                                                                                                       \
    LOG_ARG_CHECK(__VA_ARGS__)                                                                             \
    if (TRI_IsHumanLogging() && TRI_IsWarningLogging()) {                                                    \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__, TRI_LOG_LEVEL_WARNING, TRI_LOG_SEVERITY_HUMAN, __VA_ARGS__); \
    }                                                                                                        \
  } while (0)

#else

#define LOG_WARNING(...) while (0)

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs info messages
////////////////////////////////////////////////////////////////////////////////

#undef LOG_INFO

#ifdef TRI_ENABLE_LOGGER

#define LOG_INFO(...)                                                                                     \
  do {                                                                                                    \
    LOG_ARG_CHECK(__VA_ARGS__)                                                                             \
    if (TRI_IsHumanLogging() && TRI_IsInfoLogging()) {                                                    \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__, TRI_LOG_LEVEL_INFO, TRI_LOG_SEVERITY_HUMAN, __VA_ARGS__); \
    }                                                                                                     \
  } while (0)

#else

#define LOG_INFO(...) while (0)

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs debug messages
////////////////////////////////////////////////////////////////////////////////

#undef LOG_DEBUG

#ifdef TRI_ENABLE_LOGGER

#define LOG_DEBUG(...)                                                                                     \
  do {                                                                                                     \
    LOG_ARG_CHECK(__VA_ARGS__)                                                                             \
    if (TRI_IsHumanLogging() && TRI_IsDebugLogging(__FILE__)) {                                            \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__, TRI_LOG_LEVEL_DEBUG, TRI_LOG_SEVERITY_HUMAN, __VA_ARGS__); \
    }                                                                                                      \
  } while (0)

#else

#define LOG_DEBUG(...) while (0)

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs trace messages
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_ENABLE_LOGGER

#define LOG_TRACE(...)                                                                                     \
  do {                                                                                                     \
    LOG_ARG_CHECK(__VA_ARGS__)                                                                             \
    if (TRI_IsHumanLogging() && TRI_IsTraceLogging(__FILE__)) {                                            \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__, TRI_LOG_LEVEL_TRACE, TRI_LOG_SEVERITY_HUMAN, __VA_ARGS__); \
    }                                                                                                      \
  } while (0)

#else

#define LOG_TRACE(...) while (0)

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs usage messages
////////////////////////////////////////////////////////////////////////////////

#undef LOG_USAGE

#ifdef TRI_ENABLE_LOGGER

#define LOG_USAGE(...)                                                                                    \
  do {                                                                                                    \
    LOG_ARG_CHECK(__VA_ARGS__)                                                                            \
    if (TRI_IsUsageLogging()) {                                                                           \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__, TRI_LOG_LEVEL_INFO, TRI_LOG_SEVERITY_USAGE, __VA_ARGS__); \
    }                                                                                                     \
  } while (0)

#else

#define LOG_USAGE(...) while (0)

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs performance messages
////////////////////////////////////////////////////////////////////////////////

#undef LOG_ACTION

#ifdef TRI_ENABLE_LOGGER

#define LOG_ACTION(...)                                                 \
  do {                                                                  \
    LOG_ARG_CHECK(__VA_ARGS__);                                         \
    if (TRI_IsPerformanceLogging()) {                                   \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__,                         \
              TRI_LOG_LEVEL_INFO,                                       \
              TRI_LOG_SEVERITY_HUMAN,                                   \
              "[action] " __VA_ARGS__);                                 \
    }                                                                   \
  } while (0)

#else

#define LOG_PERFORMANCE(...) while (0)

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logs timings
////////////////////////////////////////////////////////////////////////////////

#undef LOG_TIMER

#ifdef TRI_ENABLE_LOGGER

#define LOG_TIMER(value, ...)                                           \
  do {                                                                  \
    LOG_ARG_CHECK(__VA_ARGS__);                                         \
    double timerValue = value;                                          \
    if (timerValue > 1.0 && TRI_IsPerformanceLogging()) {               \
      TRI_Log(__FUNCTION__, __FILE__, __LINE__,                         \
              TRI_LOG_LEVEL_INFO,                                       \
              TRI_LOG_SEVERITY_HUMAN,                                   \
              "[timer] %0.2f s - " __VA_ARGS__, timerValue);            \
    }                                                                   \
  } while (0)

#else

#define LOG_TIMER(...) while (0)

#endif


// -----------------------------------------------------------------------------
// --SECTION--                                                      LOG APPENDER
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                     private types
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief base structure for log appenders
////////////////////////////////////////////////////////////////////////////////

struct TRI_log_appender_s;

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a log append for file output
////////////////////////////////////////////////////////////////////////////////

struct TRI_log_appender_s* TRI_CreateLogAppenderFile (char const*,
                                                      char const*,
                                                      TRI_log_severity_e,
                                                      bool consume,
                                                      bool fatal2error);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a log append for syslog
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_ENABLE_SYSLOG
struct TRI_log_appender_s* TRI_CreateLogAppenderSyslog (char const*,
                                                        char const*,
                                                        char const*,
                                                        TRI_log_severity_e,
                                                        bool);
#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                            MODULE
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief return global log file name
////////////////////////////////////////////////////////////////////////////////

char const* TRI_GetFilenameLogging (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief initialises the logging components
///
/// Warning: This function call is not thread safe. Never mix it with
/// TRI_ShutdownLogging.
////////////////////////////////////////////////////////////////////////////////

void TRI_InitialiseLogging (bool threaded);

////////////////////////////////////////////////////////////////////////////////
/// @brief shut downs the logging components
///
/// Warning: This function call is not thread safe. Never mix it with
/// TRI_InitialiseLogging.
////////////////////////////////////////////////////////////////////////////////

bool TRI_ShutdownLogging (bool);

////////////////////////////////////////////////////////////////////////////////
/// @brief reopens all log appenders
////////////////////////////////////////////////////////////////////////////////

void TRI_ReopenLogging (void);

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
