/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_

#include "base/files/file_path.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_installer.h"

namespace brave_wallet {

class SnapInstallerTarDecompressor {
 public:
  SnapInstallerTarDecompressor() = delete;
  SnapInstallerTarDecompressor(const SnapInstallerTarDecompressor&) = delete;
  SnapInstallerTarDecompressor& operator=(const SnapInstallerTarDecompressor&) =
      delete;

  // Reads the gzip-compressed tarball at |tarball_path|, decompresses it,
  // extracts bundle.js and manifest.json to an unpacked temp directory,
  // computes the MetaMask checksum, and deletes |tarball_path|.
  static SnapInstaller::ExtractResult ExtractTarballToDir(
      base::FilePath tarball_path);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_TAR_DECOMPRESSOR_H_
