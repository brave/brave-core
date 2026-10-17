/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_CHECKSUM_CALCULATOR_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_CHECKSUM_CALCULATOR_H_

#include <optional>
#include <string>
#include <string_view>

namespace brave_wallet {

class SnapInstallerChecksumCalculator {
 public:
  SnapInstallerChecksumCalculator() = delete;
  SnapInstallerChecksumCalculator(const SnapInstallerChecksumCalculator&) =
      delete;
  SnapInstallerChecksumCalculator& operator=(
      const SnapInstallerChecksumCalculator&) = delete;

  static std::optional<std::string> ComputeMetaMaskChecksum(
      std::string_view decompressed_tar,
      std::string_view bundle_js,
      std::string_view bundle_file_path,
      std::string_view manifest_json);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_SNAP_INSTALLER_CHECKSUM_CALCULATOR_H_
