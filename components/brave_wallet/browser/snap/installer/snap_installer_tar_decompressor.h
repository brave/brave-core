/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_

#include <cstdint>
#include <string>

#include "base/files/file_path.h"

namespace brave_wallet {

// Result of decompressing and extracting a snap tarball.
// On success |error| is empty and the extracted files are written under
// |temp_dir_path|/unpacked/.
struct SnapTarballExtractResult {
  std::string manifest_json;  // text of snap.manifest.json
  std::string computed_shasum;
  uint64_t bundle_size_bytes = 0;
  // Base of the snap-specific temp dir created on the thread pool.
  // The actual files are under <temp_dir_path>/unpacked/.
  base::FilePath temp_dir_path;
  std::string error;
};

class SnapInstallerTarDecompressor {
 public:
  SnapInstallerTarDecompressor() = delete;
  SnapInstallerTarDecompressor(const SnapInstallerTarDecompressor&) = delete;
  SnapInstallerTarDecompressor& operator=(const SnapInstallerTarDecompressor&) =
      delete;

  // Reads the gzip-compressed tarball at |tarball_path|, decompresses it,
  // extracts bundle.js and manifest.json to an unpacked temp directory,
  // computes the MetaMask checksum, and deletes |tarball_path|.
  static SnapTarballExtractResult ExtractTarballToDir(
      base::FilePath tarball_path);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_
