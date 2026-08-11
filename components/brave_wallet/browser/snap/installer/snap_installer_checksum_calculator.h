/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_CHECKSUM_CALCULATOR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_CHECKSUM_CALCULATOR_H_

#include <string>

namespace brave_wallet {

class SnapInstallerChecksumCalculator {
 public:
  SnapInstallerChecksumCalculator() = delete;
  SnapInstallerChecksumCalculator(const SnapInstallerChecksumCalculator&) =
      delete;
  SnapInstallerChecksumCalculator& operator=(
      const SnapInstallerChecksumCalculator&) = delete;

  static std::string ComputeMetaMaskChecksum(
      const std::string& decompressed_tar,
      const std::string& bundle_js,
      const std::string& manifest_json);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_CHECKSUM_CALCULATOR_H_
