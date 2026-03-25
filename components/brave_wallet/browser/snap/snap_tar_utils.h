/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_TAR_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_TAR_UTILS_H_

#include <optional>
#include <string>

namespace brave_wallet {

// Result of extracting the two files we care about from a snap tarball.
struct SnapTarResult {
  std::string manifest_json;  // Contents of snap.manifest.json
  std::string bundle_js;      // Contents of the snap JS bundle
};

// Extracts a single file from a POSIX ustar tar archive.
//
// |path_suffix| is matched against the end of each entry's full path
// (case-sensitive). For example, "snap.manifest.json" matches
// "package/snap.manifest.json", and "dist/snap.js" matches
// "package/dist/snap.js".
//
// Returns std::nullopt if no matching entry is found or the archive is
// malformed.
std::optional<std::string> ExtractFileFromTar(const std::string& tar_data,
                                               std::string_view path_suffix);

// Parses a POSIX ustar tar archive and extracts the snap manifest and bundle.
//
// |bundle_file_path| is the relative path from the manifest's
// source.location.npm.filePath (e.g. "dist/snap.js"). The function first
// looks for a file whose path ends with that suffix. If |bundle_file_path|
// is empty it falls back to any .js file under a "dist/" directory.
//
// Returns std::nullopt if either file is absent or the archive is malformed.
std::optional<SnapTarResult> ExtractSnapFiles(const std::string& tar_data,
                                              std::string_view bundle_file_path);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_TAR_UTILS_H_
