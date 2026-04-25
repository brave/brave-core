/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/storage_utils.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "brave/browser/brave_vpn/win/wireguard_utils_win.h"
#include "components/version_info/channel.h"
#include "testing/gtest/include/gtest/gtest.h"

using version_info::Channel;

namespace brave_vpn {

TEST(StorageUtilsUnitTest, IsVPNTrayIconEnabled) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);
  // Default value is true.
  EXPECT_TRUE(IsVPNTrayIconEnabled());
  EnableVPNTrayIcon(false);
  EXPECT_FALSE(IsVPNTrayIconEnabled());
  EnableVPNTrayIcon(true);
  EXPECT_TRUE(IsVPNTrayIconEnabled());
}

TEST(StorageUtilsUnitTest, IsWireguardActive) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);
  // Default value is true.
  EXPECT_TRUE(IsWireguardActive());
  SetWireguardActive(false);
  EXPECT_FALSE(IsWireguardActive());
  SetWireguardActive(true);
  EXPECT_TRUE(IsWireguardActive());
}

TEST(StorageUtilsUnitTest, GetLastUsedConfigPath) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_LOCAL_MACHINE);
  EXPECT_FALSE(wireguard::GetLastUsedConfigPath());

  EXPECT_TRUE(wireguard::UpdateLastUsedConfigPath(base::FilePath()));
  EXPECT_FALSE(wireguard::GetLastUsedConfigPath());

  base::FilePath test_config_path(L"C:\\value");
  EXPECT_TRUE(wireguard::UpdateLastUsedConfigPath(test_config_path));
  auto last_config = wireguard::GetLastUsedConfigPath();
  EXPECT_TRUE(last_config.has_value());
  EXPECT_EQ(last_config.value(), test_config_path);
}

TEST(StorageUtilsUnitTest, ShouldFallbackToIKEv2) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_LOCAL_MACHINE);
  wireguard::SetWireguardServiceRegisteredForTesting(true);
  EXPECT_FALSE(ShouldFallbackToIKEv2());

  // By default we have limitations in 3 attempts.
  IncrementWireguardTunnelUsageFlag();
  EXPECT_FALSE(ShouldFallbackToIKEv2());
  IncrementWireguardTunnelUsageFlag();
  EXPECT_FALSE(ShouldFallbackToIKEv2());
  IncrementWireguardTunnelUsageFlag();
  EXPECT_TRUE(ShouldFallbackToIKEv2());
  ResetWireguardTunnelUsageFlag();
  EXPECT_FALSE(ShouldFallbackToIKEv2());
  wireguard::SetWireguardServiceRegisteredForTesting(false);
  EXPECT_TRUE(ShouldFallbackToIKEv2());
}

TEST(StorageUtilsUnitTest, WriteConnectionState) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);

  EXPECT_FALSE(GetConnectionState());
  WriteConnectionState(1);
  auto value = GetConnectionState();
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), 1);
}

}  // namespace brave_vpn
