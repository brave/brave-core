/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <windows.h>
#include <versionhelpers.h>
#include <winhttp.h>

#include "base/logging.h"

// Forward declaration.
namespace crashpad {
namespace {
std::string WinHttpMessage(const char* extra);
}
}

namespace {

void BraveSetSessionOptions(HINTERNET hSession) {
  // Windows 8.1+ already have TLS 1.1 and 1.2 available by default.
  if (IsWindows8Point1OrGreater())
    return;

  // Use TLS 1.0, 1.1, or 1.2.
  unsigned long secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 |
                                   WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 |
                                   WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;

  // Set protocols and log an error if we can't. Bailing out on the session due
  // to an error here is not necessary since if TLS 1.1 or 1.2 is required to
  // connect then the connection will fail anyway.
  if (!WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS,
                        &secure_protocols, sizeof(secure_protocols))) {
    LOG(ERROR) << crashpad::WinHttpMessage("WinHttpSetOption");
  }
}

}  // namespace

// The original file is patched to call the above function.
#include "src/third_party/crashpad/crashpad/util/net/http_transport_win.cc"
