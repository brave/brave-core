/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_MIME_UTIL_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_MIME_UTIL_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"

// Utilities for Mime <-> File extension conversion.
// These have more broader coverage than "net/base/mime_util.h" has as for the
// media types.
namespace playlist::mime_util {

std::optional<base::FilePath::StringType> GetFileExtensionForMimetype(
    std::string_view mime_type);

std::optional<std::string> GetMimeTypeForFileExtension(
    base::FilePath::StringPieceType file_extension);

std::vector<std::string> GetSupportedMimetypes();

}  // namespace playlist::mime_util

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_MIME_UTIL_H_
