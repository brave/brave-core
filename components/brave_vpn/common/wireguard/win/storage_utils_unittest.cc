/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#include "components/version_info/channel.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using version_info::Channel;

namespace brave_vpn {

TEST(StorageUtilsUnitTest, IsVPNTrayIconEnabled) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);
  // Default value is true.
  EXPECT_TRUE(IsVPNTrayIconEnabled(Channel::STABLE));
  EnableVPNTrayIcon(false, Channel::STABLE);
  EXPECT_FALSE(IsVPNTrayIconEnabled(Channel::STABLE));
  EnableVPNTrayIcon(true, Channel::STABLE);
  EXPECT_TRUE(IsVPNTrayIconEnabled(Channel::STABLE));
}

TEST(StorageUtilsUnitTest, IsWireguardActive) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);
  // Default value is true.
  EXPECT_TRUE(IsWireguardActive(Channel::STABLE));
  SetWireguardActive(false, Channel::STABLE);
  EXPECT_FALSE(IsWireguardActive(Channel::STABLE));
  SetWireguardActive(true, Channel::STABLE);
  EXPECT_TRUE(IsWireguardActive(Channel::STABLE));
}

TEST(StorageUtilsUnitTest, GetLastUsedConfigPath) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_LOCAL_MACHINE);
  EXPECT_FALSE(wireguard::GetLastUsedConfigPath(Channel::STABLE));

  EXPECT_TRUE(
      wireguard::UpdateLastUsedConfigPath(base::FilePath(), Channel::STABLE));
  EXPECT_FALSE(wireguard::GetLastUsedConfigPath(Channel::STABLE));

  base::FilePath test_config_path(L"C:\\value");
  EXPECT_TRUE(
      wireguard::UpdateLastUsedConfigPath(test_config_path, Channel::STABLE));
  auto last_config = wireguard::GetLastUsedConfigPath(Channel::STABLE);
  EXPECT_TRUE(last_config.has_value());
  EXPECT_EQ(last_config.value(), test_config_path);
}

TEST(StorageUtilsUnitTest, ShouldFallbackToIKEv2) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_LOCAL_MACHINE);
  wireguard::SetWireguardServiceRegisteredForTesting(true);
  EXPECT_FALSE(ShouldFallbackToIKEv2(Channel::STABLE));

  // By default we have limitations in 3 attempts.
  IncrementWireguardTunnelUsageFlag(Channel::STABLE);
  EXPECT_FALSE(ShouldFallbackToIKEv2(Channel::STABLE));
  IncrementWireguardTunnelUsageFlag(Channel::STABLE);
  EXPECT_FALSE(ShouldFallbackToIKEv2(Channel::STABLE));
  IncrementWireguardTunnelUsageFlag(Channel::STABLE);
  EXPECT_TRUE(ShouldFallbackToIKEv2(Channel::STABLE));
  ResetWireguardTunnelUsageFlag(Channel::STABLE);
  EXPECT_FALSE(ShouldFallbackToIKEv2(Channel::STABLE));
  wireguard::SetWireguardServiceRegisteredForTesting(false);
  EXPECT_TRUE(ShouldFallbackToIKEv2(Channel::STABLE));
}

TEST(StorageUtilsUnitTest, WriteConnectionState) {
  registry_util::RegistryOverrideManager registry_overrides;
  registry_overrides.OverrideRegistry(HKEY_CURRENT_USER);

  EXPECT_FALSE(GetConnectionState(Channel::STABLE));
  WriteConnectionState(1, Channel::STABLE);
  auto value = GetConnectionState(Channel::STABLE);
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), 1);
}

}  // namespace brave_vpn
