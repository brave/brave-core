/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DEFAULT_PROTOCOL_HANDLER_UTILS_WIN_H_
#define BRAVE_BROWSER_DEFAULT_PROTOCOL_HANDLER_UTILS_WIN_H_

#include <windows.h>

#include <string>

#include "base/strings/string_piece_forward.h"

namespace protocol_handler_utils {

// Exported for testing.
std::wstring GenerateUserChoiceHash(base::WStringPiece ext,
                                    base::WStringPiece sid,
                                    base::WStringPiece prog_id,
                                    SYSTEMTIME timestamp);

bool SetDefaultProtocolHandlerFor(base::WStringPiece protocol);
bool IsDefaultProtocolHandlerFor(base::WStringPiece protocol);

}  // namespace protocol_handler_utils

#endif  // BRAVE_BROWSER_DEFAULT_PROTOCOL_HANDLER_UTILS_WIN_H_
