////////////////////////////////////////////////////////////////////////////////
/// @brief arango server
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
/// @author Copyright 2011-2014, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_REST_SERVER_ARANGO_SERVER_H
#define ARANGODB_REST_SERVER_ARANGO_SERVER_H 1

#include "Basics/Common.h"

#ifdef _WIN32
  #include "Basics/win-utils.h"
#endif

#include "Rest/AnyServer.h"
#include "Rest/OperationMode.h"

#include "VocBase/vocbase.h"
#include "HttpServer/HttpHandlerFactory.h"
#include "Aql/QueryRegistry.h"

struct TRI_server_s;
struct TRI_vocbase_defaults_s;

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

namespace triagens {
  namespace rest {
    class ApplicationDispatcher;
    class ApplicationEndpointServer;
    class ApplicationScheduler;
    class AsyncJobManager;
    class HttpServer;
    class HttpsServer;
  }

  namespace admin {
    class ApplicationAdminServer;
  }

  namespace arango {
    class ApplicationMR;
    class ApplicationV8;
    class ApplicationCluster;

// -----------------------------------------------------------------------------
// --SECTION--                                                class ArangoServer
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief ArangoDB server
////////////////////////////////////////////////////////////////////////////////

  class ArangoServer : public rest::AnyServer {
      private:
        ArangoServer (const ArangoServer&);
        ArangoServer& operator= (const ArangoServer&);

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

        ArangoServer (int, char**);

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

        ~ArangoServer ();

// -----------------------------------------------------------------------------
// --SECTION--                                                 AnyServer methods
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        void buildApplicationServer ();

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        int startupServer ();

// -----------------------------------------------------------------------------
// --SECTION--                                             public static methods
// -----------------------------------------------------------------------------

      public:

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief wait for the heartbeat thread to run
/// before the server responds to requests, the heartbeat thread should have
/// run at least once
////////////////////////////////////////////////////////////////////////////////

        void waitForHeartbeat ();

////////////////////////////////////////////////////////////////////////////////
/// @brief runs in server mode
////////////////////////////////////////////////////////////////////////////////

        int runServer (struct TRI_vocbase_s*);

////////////////////////////////////////////////////////////////////////////////
/// @brief runs in console mode
////////////////////////////////////////////////////////////////////////////////

        int runConsole (struct TRI_vocbase_s*);

////////////////////////////////////////////////////////////////////////////////
/// @brief runs unit tests
////////////////////////////////////////////////////////////////////////////////

        int runUnitTests (struct TRI_vocbase_s*);

////////////////////////////////////////////////////////////////////////////////
/// @brief runs script
////////////////////////////////////////////////////////////////////////////////

        int runScript (struct TRI_vocbase_s*);

////////////////////////////////////////////////////////////////////////////////
/// @brief opens all system databases
////////////////////////////////////////////////////////////////////////////////

        void openDatabases (bool,
                            bool,
                            bool);

////////////////////////////////////////////////////////////////////////////////
/// @brief closes all database
////////////////////////////////////////////////////////////////////////////////

        void closeDatabases ();

////////////////////////////////////////////////////////////////////////////////
/// @brief defineHandlers, define "_api" and "_admin" handlers
////////////////////////////////////////////////////////////////////////////////

        void defineHandlers (triagens::rest::HttpHandlerFactory* factory);

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief number of command line arguments
////////////////////////////////////////////////////////////////////////////////

        int _argc;

////////////////////////////////////////////////////////////////////////////////
/// @brief command line arguments
////////////////////////////////////////////////////////////////////////////////

        char** _argv;

////////////////////////////////////////////////////////////////////////////////
/// @brief temporary path
////////////////////////////////////////////////////////////////////////////////

        std::string _tempPath;

////////////////////////////////////////////////////////////////////////////////
/// @brief application scheduler
////////////////////////////////////////////////////////////////////////////////

        rest::ApplicationScheduler* _applicationScheduler;

////////////////////////////////////////////////////////////////////////////////
/// @brief application dispatcher
////////////////////////////////////////////////////////////////////////////////

        rest::ApplicationDispatcher* _applicationDispatcher;

////////////////////////////////////////////////////////////////////////////////
/// @brief application endpoint server
////////////////////////////////////////////////////////////////////////////////

        rest::ApplicationEndpointServer* _applicationEndpointServer;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructed admin server application
////////////////////////////////////////////////////////////////////////////////

        admin::ApplicationAdminServer* _applicationAdminServer;

////////////////////////////////////////////////////////////////////////////////
/// @brief cluster application feature
////////////////////////////////////////////////////////////////////////////////

        triagens::arango::ApplicationCluster* _applicationCluster;

////////////////////////////////////////////////////////////////////////////////
/// @brief asynchronous job manager
////////////////////////////////////////////////////////////////////////////////

        rest::AsyncJobManager* _jobManager;

////////////////////////////////////////////////////////////////////////////////
/// @brief application V8
////////////////////////////////////////////////////////////////////////////////

        ApplicationV8* _applicationV8;

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not only requests to internal URLs need authentication
/// @startDocuBlock serverAuthenticateSystemOnly
/// `--server.authenticate-system-only boolean`
///
/// Controls whether incoming requests need authentication only if they are
/// directed to the ArangoDB's internal APIs and features, located at */_api/*,
/// */_admin/* etc.
///
/// IF the flag is set to *true*, then HTTP authentication is only
/// required for requests going to URLs starting with */_*, but not for other
/// URLs. The flag can thus be used to expose a user-made API without HTTP
/// authentication to the outside world, but to prevent the outside world from
/// using the ArangoDB API and the admin interface without authentication.
/// Note that checking the URL is performed after any database name prefix
/// has been removed. That means when the actual URL called is
/// */_db/_system/myapp/myaction*, the URL */myapp/myaction* will be used for
/// *authenticate-system-only* check.
///
/// The default is *false*.
///
/// Note that authentication still needs to be enabled for the server regularly
/// in order for HTTP authentication to be forced for the ArangoDB API and the
/// web interface.  Setting only this flag is not enough.
///
/// You can control ArangoDB's general authentication feature with the
/// *--server.disable-authentication* flag.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _authenticateSystemOnly;

////////////////////////////////////////////////////////////////////////////////
/// @brief disable authentication for ALL client requests
/// @startDocuBlock server_authentication
/// `--server.disable-authentication`
///
/// Setting value to true will turn off authentication on the server side
/// so all clients can execute any action without authorization and privilege
/// checks.
///
/// The default value is *false*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _disableAuthentication;

////////////////////////////////////////////////////////////////////////////////
/// @brief disable authentication for requests via UNIX domain sockets
/// @startDocuBlock serverAuthenticationDisable
/// `--server.disable-authentication-unix-sockets value`
///
/// Setting *value* to true will turn off authentication on the server side
/// for requests coming in via UNIX domain sockets. With this flag enabled,
/// clients located on the same host as the ArangoDB server can use UNIX domain
/// sockets to connect to the server without authentication.
/// Requests coming in by other means (e.g. TCP/IP) are not affected by this
/// option.
///
/// The default value is *false*.
///
/// **Note**: this option is only available on platforms that support UNIX domain
/// sockets.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _disableAuthenticationUnixSockets;

////////////////////////////////////////////////////////////////////////////////
/// @brief number of dispatcher threads
/// @startDocuBlock serverThreads
/// `--server.threads number`
///
/// Specifies the *number* of threads that are spawned to handle HTTP REST
/// requests.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        int _dispatcherThreads;

////////////////////////////////////////////////////////////////////////////////
/// @brief maximum size of the dispatcher queue for asynchronous requests
/// @startDocuBlock schedulerMaximalQueueSize
/// `--scheduler.maximal-queue-size size`
///
/// Specifies the maximum *size* of the dispatcher queue for asynchronous
/// task execution. If the queue already contains *size* tasks, new tasks
/// will be rejected until other tasks are popped from the queue. Setting this
/// value may help preventing from running out of memory if the queue is filled
/// up faster than the server can process requests.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        int _dispatcherQueueSize;

////////////////////////////////////////////////////////////////////////////////
/// @brief number of V8 contexts for executing JavaScript actions
/// @startDocuBlock v8Contexts
/// `--server.v8-contexts number`
///
/// Specifies the *number* of V8 contexts that are created for executing 
/// JavaScript code. More contexts allow execute more JavaScript actions in 
/// parallel, provided that there are also enough threads available. Please
/// note that each V8 context will use a substantial amount of memory and 
/// requires periodic CPU processing time for garbage collection. 
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        int _v8Contexts;

////////////////////////////////////////////////////////////////////////////////
/// @brief path to the database
/// @startDocuBlock DatabaseDirectory
/// `--database.directory directory`
///
/// The directory containing the collections and datafiles. Defaults
/// to */var/lib/arango*. When specifying the database directory, please
/// make sure the directory is actually writable by the arangod process.
///
/// You should further not use a database directory which is provided by a
/// network filesystem such as NFS. The reason is that networked filesystems
/// might cause inconsistencies when there are multiple parallel readers or
/// writers or they lack features required by arangod (e.g. flock()).
///
/// `directory`
///
/// When using the command line version, you can simply supply the database
/// directory as argument.
///
/// @EXAMPLES
///
/// ```
/// > ./arangod --server.endpoint tcp://127.0.0.1:8529 --database.directory /tmp/vocbase
/// ```
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _databasePath;

////////////////////////////////////////////////////////////////////////////////
/// @startDocuBlock databaseMaximalJournalSize
/// 
/// `--database.maximal-journal-size size`
///
/// Maximal size of journal in bytes. Can be overwritten when creating a new
/// collection. Note that this also limits the maximal size of a single
/// document.
///
/// The default is *32MB*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        TRI_voc_size_t _defaultMaximalSize;

////////////////////////////////////////////////////////////////////////////////
/// @brief default wait for sync behavior
/// @startDocuBlock databaseWaitForSync
/// `--database.wait-for-sync boolean`
///
/// Default wait-for-sync value. Can be overwritten when creating a new
/// collection.
///
/// The default is *false*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _defaultWaitForSync;

////////////////////////////////////////////////////////////////////////////////
/// @brief force syncing of collection properties to disk 
/// @startDocuBlock databaseForceSyncProperties
/// `--database.force-sync-properties boolean`
///
/// Force syncing of collection properties to disk after creating a collection
/// or updating its properties.
///
/// If turned off, no fsync will happen for the collection and database 
/// properties stored in `parameter.json` files in the file system. Turning
/// off this option will speed up workloads that create and drop a lot of
/// collections (e.g. test suites).
///
/// The default is *true*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _forceSyncProperties;

////////////////////////////////////////////////////////////////////////////////
/// @brief ignore datafile errors when loading collections
/// @startDocuBlock databaseIgnoreDatafileErrors
/// `--database.ignore-datafile-errors boolean`
///
/// If set to `false`, CRC mismatch errors in collection datafiles will lead
/// to a collection not being loaded at all. If a collection needs to be loaded
/// during WAL recovery, the WAL recovery will also abort (if not forced with
/// `--wal.ignore-recovery-errors true`). Setting this flag to `false` protects
/// users from unintentionally using a collection with corrupted datafiles, from
/// which only a subset of the original data can be recovered.
///
/// If set to `true`, CRC mismatch errors in collection datafiles will lead to
/// the datafile being partially loaded. All data up to until the mismatch will
/// be loaded. This will enable users to continue with a collection datafiles
/// that are corrupted, but will result in only a partial load of the data.
/// The WAL recovery will still abort when encountering a collection with a 
/// corrupted datafile, at least if `--wal.ignore-recovery-errors` is not set to
/// `true`.
///
/// The default value is *true*, so for collections with corrupted datafiles
/// there might be partial data loads once the WAL recovery has finished. If
/// the WAL recovery will need to load a collection with a corrupted datafile,
/// it will still stop when using the default values.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _ignoreDatafileErrors;

////////////////////////////////////////////////////////////////////////////////
/// @brief disable the replication applier on server startup
/// @startDocuBlock serverDisableReplicationApplier
/// `--server.disable-replication-applier flag`
///
/// If *true* the server will start with the replication applier turned off,
/// even if the replication applier is configured with the *autoStart* option.
/// Using the command-line option will not change the value of the *autoStart*
/// option in the applier configuration, but will suppress auto-starting the
/// replication applier just once.
///
/// If the option is not used, ArangoDB will read the applier configuration from
/// the file *REPLICATION-APPLIER-CONFIG* on startup, and use the value of the
/// *autoStart* attribute from this file.
///
/// The default is *false*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _disableReplicationApplier;

////////////////////////////////////////////////////////////////////////////////
/// @brief disable the query tracking feature
/// @startDocuBlock databaseDisableQueryTracking
/// `--database.disable-query-tracking flag`
///
/// If *true*, the server's query tracking feature will be disabled by default.
///
/// The default is *false*.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        bool _disableQueryTracking;

////////////////////////////////////////////////////////////////////////////////
/// @brief unit tests
///
/// @CMDOPT{\--javascript.unit-tests @CA{test-file}}
///
/// Runs one or more unit tests.
////////////////////////////////////////////////////////////////////////////////

        std::vector<std::string> _unitTests;

////////////////////////////////////////////////////////////////////////////////
/// @brief files to jslint
///
/// @CMDOPT{\--jslint @CA{test-file}}
///
/// Runs jslint on one or more files.
////////////////////////////////////////////////////////////////////////////////

        std::vector<std::string> _jslint;

////////////////////////////////////////////////////////////////////////////////
/// @brief run script file
///
/// @CMDOPT{\--javascript.script @CA{script-file}}
///
/// Runs the script file.
////////////////////////////////////////////////////////////////////////////////

        std::vector<std::string> _scriptFile;

////////////////////////////////////////////////////////////////////////////////
/// @brief parameters to script file
///
/// @CMDOPT{\--javascript.script-parameter @CA{script-parameter}}
///
/// Parameter to script.
////////////////////////////////////////////////////////////////////////////////

        std::vector<std::string> _scriptParameters;

////////////////////////////////////////////////////////////////////////////////
/// @brief server default language for sorting strings
/// @startDocuBlock DefaultLanguage
/// `--default-language default-language`
///
/// The default language ist used for sorting and comparing strings.
/// The language value is a two-letter language code (ISO-639) or it is
/// composed by a two-letter language code with and a two letter country code
/// (ISO-3166). Valid languages are "de", "en", "en_US" or "en_UK".
///
/// The default default-language is set to be the system locale on that platform.
/// @endDocuBlock
////////////////////////////////////////////////////////////////////////////////

        std::string _defaultLanguage;

////////////////////////////////////////////////////////////////////////////////
/// @brief the server
////////////////////////////////////////////////////////////////////////////////

        struct TRI_server_s* _server;

////////////////////////////////////////////////////////////////////////////////
/// @brief the server
////////////////////////////////////////////////////////////////////////////////

        aql::QueryRegistry* _queryRegistry;

////////////////////////////////////////////////////////////////////////////////
/// @brief ptr to pair of _applicationV8 and _queryRegistry for _api/aql handler
/// this will be removed once we have a global struct with "everything useful"
////////////////////////////////////////////////////////////////////////////////

        std::pair<ApplicationV8*, aql::QueryRegistry*>* _pairForAql;

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
