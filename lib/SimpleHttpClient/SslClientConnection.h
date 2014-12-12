////////////////////////////////////////////////////////////////////////////////
/// @brief ssl client connection
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
/// @author Jan Steemann
/// @author Copyright 2014, ArangoDB GmbH, Cologne, Germany
/// @author Copyright 2009-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_SIMPLE_HTTP_CLIENT_SSL_CLIENT_CONNECTION_H
#define ARANGODB_SIMPLE_HTTP_CLIENT_SSL_CLIENT_CONNECTION_H 1

#include "Basics/Common.h"

#include "Basics/socket-utils.h"
#include "SimpleHttpClient/GeneralClientConnection.h"

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

// -----------------------------------------------------------------------------
// --SECTION--                                               SslClientConnection
// -----------------------------------------------------------------------------

namespace triagens {
  namespace httpclient {

////////////////////////////////////////////////////////////////////////////////
/// @brief client connection
////////////////////////////////////////////////////////////////////////////////

    class SslClientConnection : public GeneralClientConnection {

// -----------------------------------------------------------------------------
// --SECTION--                                        constructors / destructors
// -----------------------------------------------------------------------------

      private:

        SslClientConnection (SslClientConnection const&);
        SslClientConnection& operator= (SslClientConnection const&);

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new client connection
////////////////////////////////////////////////////////////////////////////////

        SslClientConnection (triagens::rest::Endpoint* endpoint,
                             double,
                             double,
                             size_t,
                             uint32_t);

////////////////////////////////////////////////////////////////////////////////
/// @brief destroys a client connection
////////////////////////////////////////////////////////////////////////////////

        virtual ~SslClientConnection ();

// -----------------------------------------------------------------------------
// --SECTION--                                         protected virtual methods
// -----------------------------------------------------------------------------

      protected:

////////////////////////////////////////////////////////////////////////////////
/// @brief connect
////////////////////////////////////////////////////////////////////////////////

        bool connectSocket ();

////////////////////////////////////////////////////////////////////////////////
/// @brief disconnect
////////////////////////////////////////////////////////////////////////////////

        void disconnectSocket ();

////////////////////////////////////////////////////////////////////////////////
/// @brief prepare connection for read/write I/O
////////////////////////////////////////////////////////////////////////////////

        bool prepare (const double, const bool) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief write data to the connection
////////////////////////////////////////////////////////////////////////////////

        bool writeClientConnection (void*, size_t, size_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief read data from the connection
////////////////////////////////////////////////////////////////////////////////

        bool readClientConnection (triagens::basics::StringBuffer&, 
                                   bool& connectionClosed);

////////////////////////////////////////////////////////////////////////////////
/// @brief return whether the connection is readable
////////////////////////////////////////////////////////////////////////////////

        bool readable ();

////////////////////////////////////////////////////////////////////////////////
/// @brief return whether the socket is still workable
////////////////////////////////////////////////////////////////////////////////

        bool checkSocket ();

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief the underlying socket
////////////////////////////////////////////////////////////////////////////////

        TRI_socket_t _socket;

////////////////////////////////////////////////////////////////////////////////
/// @brief the underlying session
////////////////////////////////////////////////////////////////////////////////

        SSL* _ssl;

////////////////////////////////////////////////////////////////////////////////
/// @brief SSL context
////////////////////////////////////////////////////////////////////////////////

        SSL_CTX* _ctx;

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
