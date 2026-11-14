/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "base/files/file_path.h"
#include "base/types/expected.h"

namespace brave_wallet {

// Upper bound on both the on-disk tarball and its decompressed size. Snap
// packages are a few hundred KB; this bounds the allocation GzipUncompress
// performs from the attacker-controlled gzip size trailer.
inline constexpr size_t kMaxSnapTarballSize = 32u * 1024 * 1024;

// Files extracted from a snap tarball, written under |temp_dir_path|/unpacked/.
struct SnapTarballExtractResult {
  std::string manifest_json;  // text of snap.manifest.json
  std::string computed_shasum;
  uint64_t bundle_size_bytes = 0;
  // Base of the temp dir created for this extraction. The caller owns it on
  // success. The actual files are under <temp_dir_path>/unpacked/.
  base::FilePath temp_dir_path;
};

using SnapTarballExtractOutcome =
    base::expected<SnapTarballExtractResult, std::string>;

class SnapInstallerTarDecompressor {
 public:
  SnapInstallerTarDecompressor() = delete;
  SnapInstallerTarDecompressor(const SnapInstallerTarDecompressor&) = delete;
  SnapInstallerTarDecompressor& operator=(const SnapInstallerTarDecompressor&) =
      delete;

  // Reads the gzip-compressed tarball at |tarball_path|, decompresses it,
  // extracts bundle.js and manifest.json to an unpacked temp directory and
  // computes the MetaMask checksum. The caller owns |tarball_path| and is
  // responsible for deleting it. On success the caller also owns
  // |temp_dir_path| and must delete it recursively.
  static SnapTarballExtractOutcome ExtractTarballToDir(
      const base::FilePath& tarball_path);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_
