////////////////////////////////////////////////////////////////////////////////
/// @brief utf8 helper functions
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

#include "utf8-helper.h"

#include "unicode/unorm2.h"

// -----------------------------------------------------------------------------
// --SECTION--                                                 private functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief convert a utf-8 string to a uchar (utf-16)
////////////////////////////////////////////////////////////////////////////////

UChar* TRI_Utf8ToUChar (TRI_memory_zone_t* zone,
                        const char* utf8,
                        const size_t inLength,
                        size_t* outLength) {
  UErrorCode status;
  UChar* utf16;
  int32_t utf16Length;

  // 1. convert utf8 string to utf16
  // calculate utf16 string length
  status = U_ZERO_ERROR;
  u_strFromUTF8(nullptr, 0, &utf16Length, utf8, (int32_t) inLength, &status);
  if (status != U_BUFFER_OVERFLOW_ERROR) {
    return nullptr;
  }

  utf16 = (UChar *) TRI_Allocate(zone, (utf16Length + 1) * sizeof(UChar), false);
  if (utf16 == nullptr) {
    return nullptr;
  }

  // now convert
  status = U_ZERO_ERROR;
  // the +1 will append a 0 byte at the end
  u_strFromUTF8(utf16, utf16Length + 1, nullptr, utf8, (int32_t) inLength, &status);
  if (status != U_ZERO_ERROR) {
    TRI_Free(zone, utf16);
    return 0;
  }

  *outLength = (size_t) utf16Length;

  return utf16;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert a uchar (utf-16) to a utf-8 string
////////////////////////////////////////////////////////////////////////////////

char* TRI_UCharToUtf8 (TRI_memory_zone_t* zone,
                       const UChar* uchar,
                       const size_t inLength,
                       size_t* outLength) {
  UErrorCode status;
  char* utf8;
  int32_t utf8Length;

  // calculate utf8 string length
  status = U_ZERO_ERROR;
  u_strToUTF8(nullptr, 0, &utf8Length, uchar, (int32_t) inLength, &status);
  if (status != U_BUFFER_OVERFLOW_ERROR) {
    return nullptr;
  }

  utf8 = static_cast<char*>(TRI_Allocate(zone, (utf8Length + 1) * sizeof(char), false));

  if (utf8 == nullptr) {
    return nullptr;
  }

  // convert to utf8
  status = U_ZERO_ERROR;
  // the +1 will append a 0 byte at the end
  u_strToUTF8(utf8, utf8Length + 1, nullptr, uchar, (int32_t) inLength, &status);
  if (status != U_ZERO_ERROR) {
    TRI_Free(zone, utf8);
    return nullptr;
  }

  *outLength = ((size_t) utf8Length);

  return utf8;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief normalize an utf8 string (NFC)
////////////////////////////////////////////////////////////////////////////////

char* TRI_normalize_utf8_to_NFC (TRI_memory_zone_t* zone,
                                 char const* utf8,
                                 size_t inLength,
                                 size_t* outLength) {
  size_t utf16Length;

  *outLength = 0;
  char* utf8Dest;

  if (inLength == 0) {
    utf8Dest = static_cast<char*>(TRI_Allocate(zone, sizeof(char), false));

    if (utf8Dest != nullptr) {
      utf8Dest[0] = '\0';
    }
    return utf8Dest;
  }

  UChar* utf16 = TRI_Utf8ToUChar(zone, utf8, inLength, &utf16Length);

  if (utf16 == nullptr) {
    return nullptr;
  }

  // continue in TR_normalize_utf16_to_NFC
  utf8Dest = TRI_normalize_utf16_to_NFC(zone, (const uint16_t*) utf16, (int32_t) utf16Length, outLength);
  TRI_Free(zone, utf16);

  return utf8Dest;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief normalize an utf8 string (NFC)
////////////////////////////////////////////////////////////////////////////////

char* TRI_normalize_utf16_to_NFC (TRI_memory_zone_t* zone,
                                  uint16_t const* utf16,
                                  size_t inLength,
                                  size_t* outLength) {
  *outLength = 0;
  char* utf8Dest;

  if (inLength == 0) {
    utf8Dest = static_cast<char*>(TRI_Allocate(zone, sizeof(char), false));
    if (utf8Dest != nullptr) {
      utf8Dest[0] = '\0';
    }
    return utf8Dest;
  }

  UErrorCode status = U_ZERO_ERROR;
  const UNormalizer2* norm2 = unorm2_getInstance(nullptr, "nfc", UNORM2_COMPOSE, &status);

  if (status != U_ZERO_ERROR) {
    return nullptr;
  }

  // normalize UChar (UTF-16)
  UChar* utf16Dest;
  bool mustFree;
  char buffer[64];

  if (inLength < sizeof(buffer) / sizeof(UChar)) {
    // use a static buffer
    utf16Dest = (UChar *) &buffer[0];
    mustFree = false;
  }
  else {
    // use dynamic memory
    utf16Dest = (UChar *) TRI_Allocate(zone, (inLength + 1) * sizeof(UChar), false);
    if (utf16Dest == nullptr) {
      return nullptr;
    }
    mustFree = true;
  }

  size_t overhead = 0;
  int32_t utf16DestLength;
 
  while (true) {
    status = U_ZERO_ERROR;
    utf16DestLength = unorm2_normalize(norm2, (UChar*) utf16, (int32_t) inLength, utf16Dest, (int32_t) (inLength + overhead + 1), &status);

    if (status == U_ZERO_ERROR) {
      break;
    }

    if (status == U_BUFFER_OVERFLOW_ERROR ||
        status == U_STRING_NOT_TERMINATED_WARNING) {
      // output buffer was too small. now re-try with a bigger buffer (inLength + overhead size)
      if (mustFree) {
        // free original buffer first so we don't leak
        TRI_Free(zone, utf16Dest);
        mustFree = false;
      }
      
      if (overhead == 0) {
        // set initial overhead size
        if (inLength < 256) {
          overhead = 16;
        }
        else if (inLength < 4096) {
          overhead = 128;
        }
        else {
          overhead = 256;
        }
      }
      else {
        // use double buffer size
        overhead += overhead;

        if (overhead >= 1024 * 1024) {
          // enough is enough
          return nullptr;
        }
      }

      utf16Dest = (UChar *) TRI_Allocate(zone, (inLength + overhead + 1) * sizeof(UChar), false);

      if (utf16Dest != nullptr) {
        // got new memory. now try again with the adjusted, bigger buffer
        mustFree = true;
        continue;
      }
      // fall-through intentional
    }

    if (mustFree) {
      TRI_Free(zone, utf16Dest);
    }
    return nullptr;
  }

  // Convert data back from UChar (UTF-16) to UTF-8
  utf8Dest = TRI_UCharToUtf8(zone, utf16Dest, (size_t) utf16DestLength, outLength);

  if (mustFree) {
    TRI_Free(zone, utf16Dest);
  }

  return utf8Dest;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
