/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/resources/resource_loader.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

TEST(ResourceLoaderTest, FindPakPath) {
  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  base::FilePath wireguard = test_data_dir.Append(L"wireguard");
  // Looking to upper directory.
  EXPECT_EQ(
      FindPakFilePath(wireguard.Append(L"BraveVpnWireguardService"), "en-US"),
      base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
  // Looking to current directory.
  EXPECT_EQ(FindPakFilePath(wireguard, "en-US"),
            base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
  // Fallback to english locale.
  EXPECT_EQ(FindPakFilePath(wireguard, "de-DE"),
            base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
}

}  // namespace brave_vpn
