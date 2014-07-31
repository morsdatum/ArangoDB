////////////////////////////////////////////////////////////////////////////////
/// @brief Aql, query parser
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
/// @author Copyright 2012-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_AQL_PARSER_H
#define ARANGODB_AQL_PARSER_H 1

#include "Basics/Common.h"
#include "Aql/Ast.h"
#include "Aql/Query.h"

// -----------------------------------------------------------------------------
// --SECTION--                                                          forwards
// -----------------------------------------------------------------------------

namespace triagens {
  namespace aql {

    struct AstNode;
    class Query;
    class Parser;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief forwards for the parse function provided by the parser (.y)
////////////////////////////////////////////////////////////////////////////////

int Aqlparse (triagens::aql::Parser*);

////////////////////////////////////////////////////////////////////////////////
/// @brief forward for the init function provided by the lexer (.l)
////////////////////////////////////////////////////////////////////////////////

int Aqllex_init (void**);

////////////////////////////////////////////////////////////////////////////////
/// @brief forward for the shutdown function provided by the lexer (.l)
////////////////////////////////////////////////////////////////////////////////

int Aqllex_destroy (void *);

////////////////////////////////////////////////////////////////////////////////
/// @brief forward for the context function provided by the lexer (.l)
////////////////////////////////////////////////////////////////////////////////

void Aqlset_extra (triagens::aql::Parser*, void*);


namespace triagens {
  namespace aql {

// -----------------------------------------------------------------------------
// --SECTION--                                                      class Parser
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief the parser
////////////////////////////////////////////////////////////////////////////////

    class Parser {

// -----------------------------------------------------------------------------
// --SECTION--                                        constructors / destructors
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief create the parser
////////////////////////////////////////////////////////////////////////////////

        Parser (Query*);

////////////////////////////////////////////////////////////////////////////////
/// @brief destroy the parser
////////////////////////////////////////////////////////////////////////////////

        ~Parser ();

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief return the ast during parsing
////////////////////////////////////////////////////////////////////////////////
        
        inline Ast* ast () {
          return _ast;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief return the query during parsing
////////////////////////////////////////////////////////////////////////////////
        
        inline Query* query () {
          return _query;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief return the scanner
////////////////////////////////////////////////////////////////////////////////
        
        inline void* scanner () const {
          return _scanner;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief a pointer to the start of the query string
////////////////////////////////////////////////////////////////////////////////
        
        inline char const* queryString () const {
          return _query->queryString();
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief return the remaining length of the query string to process
////////////////////////////////////////////////////////////////////////////////

        inline size_t remainingLength () const {
          return _remainingLength;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief return the current marker position
////////////////////////////////////////////////////////////////////////////////

        inline char const* marker () const {
          return _marker;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief set the current marker position
////////////////////////////////////////////////////////////////////////////////

        inline void marker (char const* marker) {
          _marker = marker;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief return the current parse position
////////////////////////////////////////////////////////////////////////////////
        
        inline size_t offset () const {
          return _offset;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief adjust the current parse position
////////////////////////////////////////////////////////////////////////////////

        inline void increaseOffset (int offset) {
          _offset += (size_t) offset;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief fill the output buffer with a fragment of the query
////////////////////////////////////////////////////////////////////////////////

        void fillBuffer (char* result,
                         size_t length) {
          memcpy(result, _buffer, length);
          _buffer += length;
          _remainingLength -= length;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief set data for write queries
////////////////////////////////////////////////////////////////////////////////

        bool configureWriteQuery (QueryType,
                                  AstNode const*,
                                  AstNode*);

////////////////////////////////////////////////////////////////////////////////
/// @brief parse the query
////////////////////////////////////////////////////////////////////////////////

        ParseResult parse ();

////////////////////////////////////////////////////////////////////////////////
/// @brief register a parse error, position is specified as line / column
////////////////////////////////////////////////////////////////////////////////

        void registerParseError (char const*,
                                 char const*,
                                 int,
                                 int);

////////////////////////////////////////////////////////////////////////////////
/// @brief register a parse error, position is specified as line / column
////////////////////////////////////////////////////////////////////////////////

        void registerParseError (char const*,
                                 int,
                                 int);

////////////////////////////////////////////////////////////////////////////////
/// @brief register a non-parse error
////////////////////////////////////////////////////////////////////////////////

        void registerError (int,
                            char const* = nullptr);

////////////////////////////////////////////////////////////////////////////////
/// @brief push an AstNode into the list element on top of the stack
////////////////////////////////////////////////////////////////////////////////

        void pushList (AstNode*);

////////////////////////////////////////////////////////////////////////////////
/// @brief push an AstNode into the array element on top of the stack
////////////////////////////////////////////////////////////////////////////////

        void pushArray (char const*,
                        AstNode*);

////////////////////////////////////////////////////////////////////////////////
/// @brief push a temporary value on the parser's stack
////////////////////////////////////////////////////////////////////////////////

        void pushStack (void*);

////////////////////////////////////////////////////////////////////////////////
/// @brief pop a temporary value from the parser's stack
////////////////////////////////////////////////////////////////////////////////
        
        void* popStack ();

////////////////////////////////////////////////////////////////////////////////
/// @brief peek at a temporary value from the parser's stack
////////////////////////////////////////////////////////////////////////////////
        
        void* peekStack ();

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief the query
////////////////////////////////////////////////////////////////////////////////

        Query*              _query;          

////////////////////////////////////////////////////////////////////////////////
/// @brief abstract syntax tree for the query, build during parsing
////////////////////////////////////////////////////////////////////////////////

        Ast*                _ast;

////////////////////////////////////////////////////////////////////////////////
/// @brief lexer / scanner used when parsing the query (Aql/tokens.ll)
////////////////////////////////////////////////////////////////////////////////

        void*               _scanner; 

////////////////////////////////////////////////////////////////////////////////
/// @brief currently processed part of the query string
////////////////////////////////////////////////////////////////////////////////

        char const*         _buffer;    

////////////////////////////////////////////////////////////////////////////////
/// @brief remaining length of the query string, used during parsing
////////////////////////////////////////////////////////////////////////////////

        size_t              _remainingLength; 

////////////////////////////////////////////////////////////////////////////////
/// @brief current offset into query string, used during parsing
////////////////////////////////////////////////////////////////////////////////

        size_t              _offset;    

////////////////////////////////////////////////////////////////////////////////
/// @brief pointer into query string, used temporarily during parsing
////////////////////////////////////////////////////////////////////////////////

        char const*         _marker;

////////////////////////////////////////////////////////////////////////////////
/// @brief a stack of things, used temporarily during parsing
////////////////////////////////////////////////////////////////////////////////

        std::vector<void*>  _stack;
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
