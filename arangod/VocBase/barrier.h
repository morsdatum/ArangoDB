////////////////////////////////////////////////////////////////////////////////
/// @brief deletion barriers for datafiles
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

#ifndef ARANGODB_VOC_BASE_BARRIER_H
#define ARANGODB_VOC_BASE_BARRIER_H 1

#include "Basics/Common.h"

#include "Basics/locks.h"

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

struct TRI_document_collection_t;
struct TRI_collection_t;
struct TRI_datafile_s;

// -----------------------------------------------------------------------------
// --SECTION--                                                      public types
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier element type
////////////////////////////////////////////////////////////////////////////////

typedef enum {
  TRI_BARRIER_ELEMENT = 1,
  TRI_BARRIER_DATAFILE_DROP_CALLBACK,
  TRI_BARRIER_DATAFILE_RENAME_CALLBACK,
  TRI_BARRIER_COLLECTION_UNLOAD_CALLBACK,
  TRI_BARRIER_COLLECTION_DROP_CALLBACK,
  TRI_BARRIER_COLLECTION_REPLICATION,
  TRI_BARRIER_COLLECTION_COMPACTION
}
TRI_barrier_type_e;

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier element
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_s {
  struct TRI_barrier_s* _prev;
  struct TRI_barrier_s* _next;

  struct TRI_barrier_list_s* _container;

  TRI_barrier_type_e _type;
}
TRI_barrier_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier element blocker
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_blocker_s {
  TRI_barrier_t base;

  size_t        _line;
  char const*   _filename;

  uint32_t      _usedByExternal;
  bool          _usedByTransaction;
}
TRI_barrier_blocker_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier replication
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_replication_s {
  TRI_barrier_t base;
}
TRI_barrier_replication_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier compaction
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_compaction_s {
  TRI_barrier_t base;
}
TRI_barrier_compaction_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier element datafile removal callback
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_datafile_drop_cb_s {
  TRI_barrier_t base;

  struct TRI_datafile_s* _datafile;
  void* _data;
  void (*callback) (struct TRI_datafile_s*, void*);
}
TRI_barrier_datafile_drop_cb_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier element datafile rename callback
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_datafile_rename_cb_s {
  TRI_barrier_t base;

  struct TRI_datafile_s* _datafile;
  void* _data;
  void (*callback) (struct TRI_datafile_s*, void*);
}
TRI_barrier_datafile_rename_cb_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief barrier element collection callback
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_collection_cb_s {
  TRI_barrier_t base;

  struct TRI_collection_t* _collection;
  void* _data;
  bool (*callback) (struct TRI_collection_t*, void*);
}
TRI_barrier_collection_cb_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief doubly linked list of barriers
////////////////////////////////////////////////////////////////////////////////

typedef struct TRI_barrier_list_s {
  struct TRI_document_collection_t* _collection;

  TRI_spin_t _lock;

  TRI_barrier_t* _begin;
  TRI_barrier_t* _end;

  uint64_t _numBarrierElements;
}
TRI_barrier_list_t;

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief initialises a barrier list
////////////////////////////////////////////////////////////////////////////////

void TRI_InitBarrierList (TRI_barrier_list_t* container,
                          struct TRI_document_collection_t* collection);

////////////////////////////////////////////////////////////////////////////////
/// @brief destroys a barrier list
////////////////////////////////////////////////////////////////////////////////

void TRI_DestroyBarrierList (TRI_barrier_list_t* container);

////////////////////////////////////////////////////////////////////////////////
/// @brief check whether the barrier list contains an element of a certain type
////////////////////////////////////////////////////////////////////////////////

bool TRI_ContainsBarrierList (TRI_barrier_list_t* container,
                              TRI_barrier_type_e type);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new barrier element
////////////////////////////////////////////////////////////////////////////////

#define TRI_CreateBarrierElement(a, b) TRI_CreateBarrierElementZ((a), (b), __LINE__, __FILE__)

TRI_barrier_t* TRI_CreateBarrierElementZ (TRI_barrier_list_t* container,
                                          bool usedByTransaction,
                                          size_t line,
                                          char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new replication barrier
////////////////////////////////////////////////////////////////////////////////

TRI_barrier_t* TRI_CreateBarrierReplication (TRI_barrier_list_t* container);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new compaction barrier
////////////////////////////////////////////////////////////////////////////////

TRI_barrier_t* TRI_CreateBarrierCompaction (TRI_barrier_list_t* container);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new datafile deletion barrier
////////////////////////////////////////////////////////////////////////////////

TRI_barrier_t* TRI_CreateBarrierDropDatafile (TRI_barrier_list_t* container,
                                               struct TRI_datafile_s* datafile,
                                               void (*callback) (struct TRI_datafile_s*, void*),
                                               void* data);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new datafile rename barrier
////////////////////////////////////////////////////////////////////////////////

TRI_barrier_t* TRI_CreateBarrierRenameDatafile (TRI_barrier_list_t* container,
                                                struct TRI_datafile_s* datafile,
                                                void (*callback) (struct TRI_datafile_s*, void*),
                                                void* data);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new collection unload barrier
////////////////////////////////////////////////////////////////////////////////

TRI_barrier_t* TRI_CreateBarrierUnloadCollection (TRI_barrier_list_t* container,
                                                  struct TRI_collection_t* collection,
                                                  bool (*callback) (struct TRI_collection_t*, void*),
                                                  void* data);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new collection drop barrier
////////////////////////////////////////////////////////////////////////////////

TRI_barrier_t* TRI_CreateBarrierDropCollection (TRI_barrier_list_t* container,
                                                struct TRI_collection_t* collection,
                                                bool (*callback) (struct TRI_collection_t*, void*),
                                                void* data);

////////////////////////////////////////////////////////////////////////////////
/// @brief removes and frees a barrier element or datafile deletion marker
////////////////////////////////////////////////////////////////////////////////

void TRI_FreeBarrier (TRI_barrier_t* element);

////////////////////////////////////////////////////////////////////////////////
/// @brief removes and frees a barrier element or datafile deletion marker,
/// this is used for barriers used by transaction or by external to protect
/// the flags by the lock
////////////////////////////////////////////////////////////////////////////////

void TRI_FreeBarrier (TRI_barrier_blocker_t* element, bool fromTransaction);

////////////////////////////////////////////////////////////////////////////////
/// @brief track usage of a barrier by an external
////////////////////////////////////////////////////////////////////////////////

void TRI_SetUsedByExternalBarrier (TRI_barrier_blocker_t* element);

////////////////////////////////////////////////////////////////////////////////
/// @brief track usage of a barrier by a transaction
////////////////////////////////////////////////////////////////////////////////

void TRI_SetUsedByTransactionBarrier (TRI_barrier_blocker_t* element);

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
