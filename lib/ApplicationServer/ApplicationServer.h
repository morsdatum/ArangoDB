////////////////////////////////////////////////////////////////////////////////
/// @brief application server
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
/// @author Copyright 2009-2014, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_APPLICATION_SERVER_APPLICATION_SERVER_H
#define ARANGODB_APPLICATION_SERVER_APPLICATION_SERVER_H 1

#include "Basics/Common.h"

#include "Basics/ProgramOptions.h"
#include "Basics/ConditionVariable.h"

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

namespace triagens {
  namespace basics {
    class ProgramOptionsDescription;
  }

  namespace rest {
    class ApplicationFeature;

// -----------------------------------------------------------------------------
// --SECTION--                                           class ApplicationServer
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief application server
////////////////////////////////////////////////////////////////////////////////

    class ApplicationServer {
      ApplicationServer (const ApplicationServer&);
      ApplicationServer& operator= (const ApplicationServer&);

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

        ApplicationServer (std::string const& name, std::string const& title, std::string const& version);

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

        virtual ~ApplicationServer ();

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief adds a new feature
////////////////////////////////////////////////////////////////////////////////

        void addFeature (ApplicationFeature*);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the name of the system config file
////////////////////////////////////////////////////////////////////////////////

        void setSystemConfigFile (const std::string& name);

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the name of the user config file
////////////////////////////////////////////////////////////////////////////////

        void setUserConfigFile (const std::string& name);

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the name of the application
////////////////////////////////////////////////////////////////////////////////

        std::string const& getName () const;

////////////////////////////////////////////////////////////////////////////////
/// @brief sets up the logging
////////////////////////////////////////////////////////////////////////////////

        void setupLogging (bool thread, bool daemon, bool backgrounded);

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the command line options
////////////////////////////////////////////////////////////////////////////////

        basics::ProgramOptions& programOptions ();

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the command line arguments
////////////////////////////////////////////////////////////////////////////////

        std::vector<std::string> programArguments ();

////////////////////////////////////////////////////////////////////////////////
/// @brief parses the arguments with empty options description
////////////////////////////////////////////////////////////////////////////////

        bool parse (int argc, char* argv[]);

////////////////////////////////////////////////////////////////////////////////
/// @brief parses the arguments
////////////////////////////////////////////////////////////////////////////////

        bool parse (int argc, char* argv[], std::map<std::string, triagens::basics::ProgramOptionsDescription>);

////////////////////////////////////////////////////////////////////////////////
/// @brief prepares the server
////////////////////////////////////////////////////////////////////////////////

        void prepare ();

////////////////////////////////////////////////////////////////////////////////
/// @brief prepares the server
////////////////////////////////////////////////////////////////////////////////

        void prepare2 ();

////////////////////////////////////////////////////////////////////////////////
/// @brief starts the scheduler
////////////////////////////////////////////////////////////////////////////////

        void start ();

////////////////////////////////////////////////////////////////////////////////
/// @brief waits for shutdown
////////////////////////////////////////////////////////////////////////////////

        void wait ();

////////////////////////////////////////////////////////////////////////////////
/// @brief begins shutdown sequence
////////////////////////////////////////////////////////////////////////////////

        void beginShutdown ();

////////////////////////////////////////////////////////////////////////////////
/// @brief stops everything
////////////////////////////////////////////////////////////////////////////////

        void stop ();

////////////////////////////////////////////////////////////////////////////////
/// @brief raises the privileges
////////////////////////////////////////////////////////////////////////////////

        void raisePrivileges ();

////////////////////////////////////////////////////////////////////////////////
/// @brief drops the privileges
////////////////////////////////////////////////////////////////////////////////

        void dropPrivileges ();

////////////////////////////////////////////////////////////////////////////////
/// @brief drops the privileges permanently
////////////////////////////////////////////////////////////////////////////////

        void dropPrivilegesPermanently ();

// -----------------------------------------------------------------------------
// --SECTION--                                                 protected methods
// -----------------------------------------------------------------------------

      protected:

////////////////////////////////////////////////////////////////////////////////
/// @brief adds options to description
////////////////////////////////////////////////////////////////////////////////

        void setupOptions (std::map<std::string, basics::ProgramOptionsDescription>&);

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if the parent is still alive
////////////////////////////////////////////////////////////////////////////////

        bool checkParent ();

////////////////////////////////////////////////////////////////////////////////
/// @brief reads the configuration files
////////////////////////////////////////////////////////////////////////////////

        bool readConfigurationFile ();

////////////////////////////////////////////////////////////////////////////////
/// @brief extracts the real privileges
////////////////////////////////////////////////////////////////////////////////

        void storeRealPrivileges ();

////////////////////////////////////////////////////////////////////////////////
/// @brief extracts the privileges from the command-line
////////////////////////////////////////////////////////////////////////////////

        void extractPrivileges ();

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief program options
/// @startDocuBlock generalHelp
/// `--help`
///
/// `-h`
///
/// Prints a list of the most common options available and then
/// exits. In order to see all options use *--help-all*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        basics::ProgramOptions _options;


////////////////////////////////////////////////////////////////////////////////
/// @brief command line option description
////////////////////////////////////////////////////////////////////////////////

        basics::ProgramOptionsDescription _description;

////////////////////////////////////////////////////////////////////////////////
/// @brief config file option description
////////////////////////////////////////////////////////////////////////////////

        basics::ProgramOptionsDescription _descriptionFile;

////////////////////////////////////////////////////////////////////////////////
/// @brief command line arguments
////////////////////////////////////////////////////////////////////////////////

        std::vector<std::string> _arguments;

////////////////////////////////////////////////////////////////////////////////
/// @brief defined features
////////////////////////////////////////////////////////////////////////////////

        std::vector<ApplicationFeature*> _features;

////////////////////////////////////////////////////////////////////////////////
/// @brief exit on parent death
////////////////////////////////////////////////////////////////////////////////

        bool _exitOnParentDeath;

////////////////////////////////////////////////////////////////////////////////
/// @brief watch parent
////////////////////////////////////////////////////////////////////////////////

        int _watchParent;

////////////////////////////////////////////////////////////////////////////////
/// @brief shutdown requested
////////////////////////////////////////////////////////////////////////////////

        volatile sig_atomic_t _stopping;

////////////////////////////////////////////////////////////////////////////////
/// @brief name of the application
////////////////////////////////////////////////////////////////////////////////

        std::string _name;

////////////////////////////////////////////////////////////////////////////////
/// @brief title of the application
////////////////////////////////////////////////////////////////////////////////

        std::string _title;

////////////////////////////////////////////////////////////////////////////////
/// @brief version of the application
/// @startDocuBlock generalVersion
/// `--version`
///
/// `-v`
///
/// Prints the version of the server and exits.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _version;

////////////////////////////////////////////////////////////////////////////////
/// @brief config file
/// @startDocuBlock configurationFilename
/// `--configuration filename`
///
/// `-c filename`
///
/// Specifies the name of the configuration file to use.
///
/// If this command is not passed to the server, then by default, the server
/// will attempt to first locate a file named *~/.arango/arangod.conf* in the
/// user's home directory.
///
/// If no such file is found, the server will proceed to look for a file
/// *arangod.conf* in the system configuration directory. The system
/// configuration directory is platform-specific, and may be changed when
/// compiling ArangoDB yourself. It may default to */etc/arangodb* or
/// */usr/local/etc/arangodb*. This file is installed when using a package
/// manager like rpm or dpkg. If you modify this file and later upgrade to a new
/// version of ArangoDB, then the package manager normally warns you about the
/// conflict. In order to avoid these warning for small adjustments, you can put
/// local overrides into a file *arangod.conf.local*.
///
/// Only command line options with a value should be set within the
/// configuration file. Command line options which act as flags should be
/// entered on the command line when starting the server.
///
/// Whitespace in the configuration file is ignored. Each option is specified on
/// a separate line in the form
///
/// ```js
/// key = value 
/// ```
///
/// Alternatively, a header section can be specified and options pertaining to
/// that section can be specified in a shorter form
///
/// ```js
/// [log] 
/// level = trace
/// ```
///
/// rather than specifying
///
/// ```js
/// log.level = trace 
/// ```
///
/// Comments can be placed in the configuration file, only if the line begins
/// with one or more hash symbols (#).
///
/// There may be occasions where a configuration file exists and the user wishes
/// to override configuration settings stored in a configuration file. Any
/// settings specified on the command line will overwrite the same setting when
/// it appears in a configuration file. If the user wishes to completely ignore
/// configuration files without necessarily deleting the file (or files), then
/// add the command line option
///
/// ```js
/// -c none 
/// ```
///
/// or
///
/// ```js
/// --configuration none 
/// ```
///
/// When starting up the server. Note that, the word *none* is
/// case-insensitive.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _configFile;

////////////////////////////////////////////////////////////////////////////////
/// @brief user config file
////////////////////////////////////////////////////////////////////////////////

        std::string _userConfigFile;

////////////////////////////////////////////////////////////////////////////////
/// @brief system config file
////////////////////////////////////////////////////////////////////////////////

        std::string _systemConfigFile;

////////////////////////////////////////////////////////////////////////////////
/// @brief the user id to use for the process
/// @startDocuBlock configurationUid
/// `--uid uid`
///
/// The name (identity) of the user the server will run as. If this parameter is
/// not specified, the server will not attempt to change its UID, so that the
/// UID used by the server will be the same as the UID of the user who started
/// the server. If this parameter is specified, then the server will change its
/// UID after opening ports and reading configuration files, but before
/// accepting connections or opening other files (such as recovery files). This
/// is useful when the server must be started with raised privileges (in certain
/// environments) but security considerations require that these privileges be
/// dropped once the server has started work.
///
/// Observe that this parameter cannot be used to bypass operating system
/// security. In general, this parameter (and its corresponding relative gid)
/// can lower privileges but not raise them.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _uid;

////////////////////////////////////////////////////////////////////////////////
/// @brief real uid
////////////////////////////////////////////////////////////////////////////////

        TRI_uid_t _realUid;

////////////////////////////////////////////////////////////////////////////////
/// @brief effective uid
////////////////////////////////////////////////////////////////////////////////

        TRI_uid_t _effectiveUid;

////////////////////////////////////////////////////////////////////////////////
/// @brief the group id to use for the process
/// @startDocuBlock configurationGid
/// `--gid gid`
///
/// The name (identity) of the group the server will run as. If this parameter
/// is not specified, then the server will not attempt to change its GID, so
/// that the GID the server runs as will be the primary group of the user who
/// started the server. If this parameter is specified, then the server will
/// change its GID after opening ports and reading configuration files, but
/// before accepting connections or opening other files (such as recovery
/// files).
///
/// This parameter is related to the parameter uid.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _gid;

////////////////////////////////////////////////////////////////////////////////
/// @brief real gid
////////////////////////////////////////////////////////////////////////////////

        TRI_gid_t _realGid;

////////////////////////////////////////////////////////////////////////////////
/// @brief effective gid
////////////////////////////////////////////////////////////////////////////////

        TRI_gid_t _effectiveGid;

////////////////////////////////////////////////////////////////////////////////
/// @brief log application name
/// @startDocuBlock logApplication
/// `--log.application name`
///
/// Specifies the *name* of the application which should be logged if this item of
/// information is to be logged.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logApplicationName;

////////////////////////////////////////////////////////////////////////////////
/// @brief log facility
/// @startDocuBlock logFacility
/// `--log.facility name`
///
/// If this option is set, then in addition to output being directed to the
/// standard output (or to a specified file, in the case that the command line
/// log.file option was set), log output is also sent to the system logging
/// facility. The *arg* is the system log facility to use. See syslog for
/// further details.
///
/// The value of *arg* depends on your syslog configuration. In general it
/// will be *user*. Fatal messages are mapped to *crit*, so if *arg*
/// is *user*, these messages will be logged as *user.crit*.  Error
/// messages are mapped to *err*.  Warnings are mapped to *warn*.  Info
/// messages are mapped to *notice*.  Debug messages are mapped to
/// *info*.  Trace messages are mapped to *debug*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logFacility;

////////////////////////////////////////////////////////////////////////////////
/// @brief log level
/// @startDocuBlock logLevel
/// `--log.level level`
///
/// `--log level`
///
/// Allows the user to choose the level of information which is logged by the
/// server. The argument *level* is specified as a string and can be one of
/// the values listed below. Note that, fatal errors, that is, errors which
/// cause the server to terminate, are always logged irrespective of the log
/// level assigned by the user. The variant *c* log.level can be used in
/// configuration files, the variant *c* log for command line options.
///
/// **fatal**: 
/// Logs errors which cause the server to terminate.
///
/// Fatal errors generally indicate some inconsistency with the manner in which
/// the server has been coded. Fatal errors may also indicate a problem with the
/// platform on which the server is running. Fatal errors always cause the
/// server to terminate. For example,
///
/// ```
/// 2010-09-20T07:32:12Z [4742] FATAL a http server has already been created
/// ```
///
/// **error**: 
/// Logs errors which the server has encountered.
///
/// These errors may not necessarily result in the termination of the
/// server. For example,
///
/// ```
/// 2010-09-17T13:10:22Z [13967] ERROR strange log level 'errors'\, going to 'warning'
/// ```
///
/// **warning**:
/// Provides information on errors encountered by the server,
/// which are not necessarily detrimental to it's continued operation.
///
/// For example,
///
/// ```
/// 2010-09-20T08:15:26Z [5533] WARNING got corrupted HTTP request 'POS?'
/// ```
///
/// **Note**: The setting the log level to warning will also result in all errors
/// to be logged as well.
///
/// **info**: 
/// Logs information about the status of the server.
///
/// For example,
///
/// ```
/// 2010-09-20T07:40:38Z [4998] INFO SimpleVOC ready for business
/// ```
///
/// **Note**: The setting the log level to info will also result in all errors and
/// warnings to be logged as well.
///
/// **debug**: 
/// Logs all errors, all warnings and debug information.
///
/// Debug log information is generally useful to find out the state of the
/// server in the case of an error. For example,
///
/// ```
/// 2010-09-17T13:02:53Z [13783] DEBUG opened port 7000 for any
/// ```
///
/// **Note**: The setting the log level to debug will also result in all errors,
/// warnings and server status information to be logged as well.
///
/// **trace**: 
/// As the name suggests, logs information which may be useful to trace
/// problems encountered with using the server.
///
/// For example,
///
/// ```
/// 2010-09-20T08:23:12Z [5687] TRACE trying to open port 8000
/// ```
///
/// **Note**: The setting the log level to trace will also result in all errors,
/// warnings, status information, and debug information to be logged as well.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logLevel;

////////////////////////////////////////////////////////////////////////////////
/// @brief log severity
/// @startDocuBlock logSeverity
/// `--log.severity severity`
///
/// This parameter provides a set of standard log severities which can be
/// used. The currently accepted *severities* are:
///
/// - exception
/// - technical
/// - functional
/// - development
/// - human
/// - all (human and non-human)
/// - non-human (exception, technical, functional, and development)
///
/// The default is all.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logSeverity;

////////////////////////////////////////////////////////////////////////////////
/// @brief log file
/// @startDocuBlock logFile
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logFile;

////////////////////////////////////////////////////////////////////////////////
/// @brief log file
////////////////////////////////////////////////////////////////////////////////

        std::string _logTty;

////////////////////////////////////////////////////////////////////////////////
/// @brief log file for requests
/// @startDocuBlock logRequests
/// `--log.requests-file filename`
///
/// This option allows the user to specify the name of a file to which
/// requests are logged. By default, no log file is used and requests are
/// not logged. Note that if the file named by *filename* does not
/// exist, it will be created. If the file cannot be created (e.g. due to
/// missing file privileges), the server will refuse to start. If the specified
/// file already exists, output is appended to that file.
///
/// Use *+* to log to standard error. Use *-* to log to standard output.
/// Use *""* to disable request logging altogether.
///
/// The log format is
/// - `"http-request"`: static string indicating that an HTTP request was logged
/// - client address: IP address of client
/// - HTTP method type, e.g. `GET`, `POST`
/// - HTTP version, e.g. `HTTP/1.1`
/// - HTTP response code, e.g. 200
/// - request body length in bytes
/// - response body length in bytes
/// - server request processing time, containing the time span between fetching
///   the first byte of the HTTP request and the start of the HTTP response
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logRequestsFile;

////////////////////////////////////////////////////////////////////////////////
/// @brief log prefix
/// @startDocuBlock logPrefix
/// `--log.prefix prefix`
///
/// This option is used specify an prefix to logged text.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logPrefix;

////////////////////////////////////////////////////////////////////////////////
/// @brief log thread identifier
/// @startDocuBlock logThread
/// `--log.thread`
///
/// Whenever log output is generated, the process ID is written as part of the
/// log information. Setting this option appends the thread id of the calling
/// thread to the process id. For example,
///
/// ```
/// 2010-09-20T13:04:01Z [19355] INFO ready for business
/// ```
///
/// when no thread is logged and
///
/// ```
/// 2010-09-20T13:04:17Z [19371-18446744072487317056] ready for business
/// ```
///
/// when this command line option is set.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _logThreadId;

////////////////////////////////////////////////////////////////////////////////
/// @brief log line number
/// @startDocuBlock logLineNumber
/// `--log.line-number`
///
/// Normally, if an human readable fatal, error, warning or info message is
/// logged, no information about the file and line number is provided. The file
/// and line number is only logged for debug and trace message. This option can
/// be use to always log these pieces of information.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _logLineNumber;

////////////////////////////////////////////////////////////////////////////////
/// @brief log dates and times in local time zone
/// @startDocuBlock logLocalTime
/// `--log.use-local-time`
///
/// If specified, all dates and times in log messages will use the server's
/// local time-zone. If not specified, all dates and times in log messages
/// will be printed in UTC / Zulu time. The date and time format used in logs 
/// is always `YYYY-MM-DD HH:MM:SS`, regardless of this setting. If UTC time
/// is used, a `Z` will be appended to indicate Zulu time.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////
        
        bool _logLocalTime;

////////////////////////////////////////////////////////////////////////////////
/// @brief log source filter
/// @startDocuBlock logSourceFilter
/// `--log.source-filter arg`
///
/// For debug and trace messages, only log those messages originated from the
/// C source file *arg*. The argument can be used multiple times.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::vector<std::string> _logSourceFilter;

////////////////////////////////////////////////////////////////////////////////
/// @brief log content filter
/// @startDocuBlock logContentFilter
/// `--log.content-filter arg`
///
/// Only log message containing the specified string *arg*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _logContentFilter;

////////////////////////////////////////////////////////////////////////////////
/// @brief performance logging
/// @startDocuBlock logPerformance
/// `--log.performance`
///
/// If this option is set, performance-related info messages will be logged via
/// the regular logging mechanisms. These will consist of mostly timing and 
/// debugging information for performance-critical operations.
/// 
/// Currently performance-related operations are logged as INFO messages.
/// Messages starting with prefix `[action]` indicate that an instrumented
/// operation was started (note that its end won't be logged). Messages with
/// prefix `[timer]` will contain timing information for operations. Note that
/// no timing information will be logged for operations taking less time than
/// 1 second. This is to ensure that sub-second operations do not pollute logs.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief random number generator to use
/// @startDocuBlock randomGenerator
/// `--random.generator arg`
///
/// The argument is an integer (1,2,3 or 4) which sets the manner in which
/// random numbers are generated. The default method (3) is to use the a
/// non-blocking random (or pseudorandom) number generator supplied by the
/// operating system. 
///
/// Specifying an argument of 2, uses a blocking random (or
/// pseudorandom) number generator. Specifying an argument 1 sets a pseudorandom
/// number generator using an implication of the Mersenne Twister MT19937
/// algorithm. Algorithm 4 is a combination of the blocking random number
/// generator and the Mersenne Twister.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        uint32_t _randomGenerator;

////////////////////////////////////////////////////////////////////////////////
/// @brief condition variable for done
////////////////////////////////////////////////////////////////////////////////

        triagens::basics::ConditionVariable _finishedCondition;
    };
  }
}

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
