/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_firewall.h"

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::wireguard {

class WireguardFirewallWinTest : public testing::Test {
 protected:
  void SetUp() override {
    // Ensure Winsock is initialized for socket-based WFP regression tests.
    WSADATA wsa_data;
    ASSERT_EQ(WSAStartup(MAKEWORD(2, 2), &wsa_data), 0);
  }

  void TearDown() override { WSACleanup(); }
};

TEST_F(WireguardFirewallWinTest, GetTunnelInterfaceAlias) {
  // Verifies the alias derivation logic removes the extension correctly.
  base::FilePath config(L"C:\\VPN\\Configs\\my_secure_tunnel.conf");
  EXPECT_EQ(GetTunnelInterfaceAlias(config), L"my_secure_tunnel");

  base::FilePath no_ext(L"C:\\VPN\\Configs\\just_name");
  EXPECT_EQ(GetTunnelInterfaceAlias(no_ext), L"just_name");
}

TEST_F(WireguardFirewallWinTest, FirewallCreationAndTeardown) {
  // Tests Phase 1 of the firewall lifecycle (Global Filters).
  // Requires Administrator privileges to interface with the BFE/WFP engine.
  auto firewall = ScopedWireguardFirewall::Create();
  if (!firewall) {
    GTEST_SKIP() << "Skipping WFP test: Administrator privileges required to "
                    "install filters.";
  }

  // Destruction of `firewall` here automatically safely tears down the dynamic
  // WFP session, effectively testing the destructor.
}

TEST_F(WireguardFirewallWinTest, PermitTunnelInterface) {
  // Tests Phase 2 of the firewall lifecycle (Tunnel Filters).
  auto firewall = ScopedWireguardFirewall::Create();
  if (!firewall) {
    GTEST_SKIP() << "Skipping WFP test: Administrator privileges required.";
  }

  NET_LUID fake_luid = {};
  fake_luid.Value = 123456789;  // Mock LUID for the test

  // Should successfully transition from Phase 1 to Phase 2.
  EXPECT_TRUE(firewall->PermitTunnelInterface(fake_luid));
}

TEST_F(WireguardFirewallWinTest, WithdrawTemporaryDnsIdempotent) {
  // Verifies that the explicit teardown method is safe to call multiple times
  // on failure paths without causing WFP transaction errors.
  auto firewall = ScopedWireguardFirewall::Create();
  if (!firewall) {
    GTEST_SKIP() << "Skipping WFP test: Administrator privileges required.";
  }

  firewall->WithdrawTemporaryDns();
  firewall->WithdrawTemporaryDns();  // Second call should not crash.
}

TEST_F(WireguardFirewallWinTest, RequestTunnelShutdown) {
  // Verifies SCM communication gracefully fails when run unprivileged or
  // when the actual service isn't installed during a unit test.
  // It shouldn't crash the test runner.
  // NOTE: This talks to the real SCM and the real service name from
  // GetBraveVpnWireguardTunnelServiceName(). On a normal dev/CI machine
  // that service isn't installed so this reliably returns false. However,
  // running this elevated on a machine with the Brave VPN Wireguard tunnel
  // installed would send a real SERVICE_CONTROL_STOP to it.
  bool result = RequestTunnelShutdown();
  // Unless the test is run as SYSTEM with the service installed, this should
  // fail.
  EXPECT_FALSE(result);
}

TEST_F(WireguardFirewallWinTest, TunnelInterfaceWatcherInitialization) {
  // Verifies the MIB notification listener can be established.
  auto watcher = TunnelInterfaceWatcher::Create(
      L"mock_adapter", base::BindOnce([](const NET_LUID& luid) {}));

  // Create() should successfully register NotifyIpInterfaceChange unless
  // the OS IP Helper API is severely degraded.
  EXPECT_NE(watcher, nullptr);
}

}  // namespace brave_vpn::wireguard
