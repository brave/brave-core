/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_TAR_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_TAR_UTILS_H_

#include <optional>
#include <string>
#include <string_view>

namespace brave_wallet {

// Result of extracting the two files we care about from a snap tarball.
struct SnapTarResult {
  std::string manifest_json;  // Contents of snap.manifest.json
  std::string bundle_js;      // Contents of the snap JS bundle
};

// Extracts a single file from a POSIX ustar tar archive.
//
// |relative_path| is matched exactly against each entry's path with the
// archive's root directory component stripped (e.g. npm tarballs wrap
// everything in "package/"). For example, "snap.manifest.json" matches
// "package/snap.manifest.json" but not "package/nested/snap.manifest.json",
// and "dist/snap.js" does not match "package/dist/snap.js" when given as
// "snap.js".
//
// Entries outside a root directory, and entries whose relative path is
// absolute or contains a ".." component, are ignored.
//
// Returns std::nullopt if no matching entry is found or the archive is
// malformed.
std::optional<std::string> ExtractFileFromTar(const std::string& tar_data,
                                              std::string_view relative_path);

// Parses a POSIX ustar tar archive and extracts the snap manifest and bundle.
//
// |bundle_file_path| is the manifest's source.location.npm.filePath (e.g.
// "dist/snap.js") and must be non-empty; it is matched exactly against each
// entry's root-relative path, per ExtractFileFromTar above.
//
// Returns std::nullopt if either file is absent or the archive is malformed.
std::optional<SnapTarResult> ExtractSnapFiles(
    const std::string& tar_data,
    std::string_view bundle_file_path);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_TAR_UTILS_H_
