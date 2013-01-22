////////////////////////////////////////////////////////////////////////////////
/// @brief file functions
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2012 triagens GmbH, Cologne, Germany
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
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2011-2012, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_BASICS_C_FILES_H
#define TRIAGENS_BASICS_C_FILES_H 1

#ifdef _WIN32
 #include "BasicsC/win-utils.h"
#endif

#include "BasicsC/common.h"

#include "BasicsC/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Files
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief sets close-on-exit for a socket
////////////////////////////////////////////////////////////////////////////////

bool TRI_SetCloseOnExecFile (socket_t fd);

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the size of a file
///
/// Will return a negative error number on error
////////////////////////////////////////////////////////////////////////////////

int64_t TRI_SizeFile (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if file or directory is writable
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsWritable (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if path is a directory
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsDirectory (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if path is a symbolic link
////////////////////////////////////////////////////////////////////////////////

bool TRI_IsSymbolicLink (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief checks if file exists
////////////////////////////////////////////////////////////////////////////////

bool TRI_ExistsFile (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a directory
////////////////////////////////////////////////////////////////////////////////

bool TRI_CreateDirectory (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief removes an empty directory
////////////////////////////////////////////////////////////////////////////////

int TRI_RemoveEmptyDirectory (char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief removes a directory recursively
////////////////////////////////////////////////////////////////////////////////

int TRI_RemoveDirectory (char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief extracts the dirname
////////////////////////////////////////////////////////////////////////////////

char* TRI_Dirname (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief extracts the basename
////////////////////////////////////////////////////////////////////////////////

char* TRI_Basename (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a filename
////////////////////////////////////////////////////////////////////////////////

char* TRI_Concatenate2File (char const* path, char const* name);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a filename
////////////////////////////////////////////////////////////////////////////////

char* TRI_Concatenate3File (char const* path1, char const* path2, char const* name);

////////////////////////////////////////////////////////////////////////////////
/// @brief returns a list of files in path
////////////////////////////////////////////////////////////////////////////////

TRI_vector_string_t TRI_FilesDirectory (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief lists the directory tree including files and directories
////////////////////////////////////////////////////////////////////////////////

TRI_vector_string_t TRI_FullTreeDirectory (char const* path);

////////////////////////////////////////////////////////////////////////////////
/// @brief renames a file
////////////////////////////////////////////////////////////////////////////////

int TRI_RenameFile (char const* old, char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief unlinks a file
////////////////////////////////////////////////////////////////////////////////

int TRI_UnlinkFile (char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief reads into a buffer from a file
////////////////////////////////////////////////////////////////////////////////

bool TRI_ReadPointer (int fd, void* buffer, size_t length);

////////////////////////////////////////////////////////////////////////////////
/// @brief writes buffer to a file
////////////////////////////////////////////////////////////////////////////////

bool TRI_WritePointer (int fd, void const* buffer, size_t length);

////////////////////////////////////////////////////////////////////////////////
/// @brief fsyncs a file
////////////////////////////////////////////////////////////////////////////////

bool TRI_fsync (int fd);

////////////////////////////////////////////////////////////////////////////////
/// @brief slurps in a file
////////////////////////////////////////////////////////////////////////////////

char* TRI_SlurpFile (TRI_memory_zone_t*, char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a lock file based on the PID
///
/// Creates a file containing a the current process identifier and locks
/// that file. Under Unix the call uses the @FN{open} system call with
/// O_EXCL to ensure that the file is created atomically. Then the
/// file is filled with the process identifier as decimal number and a
/// lock on the file is obtained using @FN{flock}.
///
/// On success @ref TRI_ERROR_NO_ERROR is returned.
///
/// Internally, the functions keeps a list of open pid files. Calling the
/// function twice with the same @FA{filename} will succeed and will not
/// create a new entry in this list. The system uses @FN{atexit} to release
/// all open locks upon exit.
////////////////////////////////////////////////////////////////////////////////

int TRI_CreateLockFile (char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief verifies a lock file based on the PID
///
/// The function checks if the file named @FA{filename} exists. If the
/// file exists, then the following checks are performed:
///
/// - Does the file contain a valid decimal number?
/// - Does this number belong to a living process?
/// - Is it possible to lock the file using @FN{flock}. This should failed.
///   If the lock can be obtained, then it is assume that the lock is invalid.
///
/// If the verification returns an error, than @FN{TRI_UnlinkFile} should be
/// used to remove the lock file. If the verification returns @ref
/// TRI_ERROR_NO_ERROR than the file is locked and the lock is valid.
////////////////////////////////////////////////////////////////////////////////

int TRI_VerifyLockFile (char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief releases a lock file based on the PID
////////////////////////////////////////////////////////////////////////////////

int TRI_DestroyLockFile (char const* filename);

////////////////////////////////////////////////////////////////////////////////
/// @brief return the absolute path of a file
/// It is the caller's responsibility to free the string created by this 
/// function
////////////////////////////////////////////////////////////////////////////////

char* TRI_GetAbsolutePath (char const*, char const*);

////////////////////////////////////////////////////////////////////////////////
/// @brief locates the directory containing the program
////////////////////////////////////////////////////////////////////////////////

char* TRI_LocateBinaryPath (char const* argv0);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:
