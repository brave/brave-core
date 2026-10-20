// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FILENAME_SANITIZER_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FILENAME_SANITIZER_H_

#include <string>
#include <string_view>

namespace brave_transfer {

// Receiver-side file name sanitization (PROTOCOL.md §7). The result is never
// empty, never longer than 255 UTF-8 bytes, and contains no path separators.
// `name` must be UTF-8; malformed sequences become U+FFFD.
std::string SanitizeFilename(std::string_view name);

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_FILENAME_SANITIZER_H_
