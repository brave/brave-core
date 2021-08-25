/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MICROSOFT_EDGE_PROTOCOL_UTIL_H_
#define BRAVE_BROWSER_MICROSOFT_EDGE_PROTOCOL_UTIL_H_

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

// Returns url if |command_line_arg| is microsoft-edge protocol args and that
// args has link info. If args is not a valid url, returns nullopt.
absl::optional<GURL> GetURLFromMSEdgeProtocol(
    base::WStringPiece command_line_arg);

#endif  // BRAVE_BROWSER_MICROSOFT_EDGE_PROTOCOL_UTIL_H_
