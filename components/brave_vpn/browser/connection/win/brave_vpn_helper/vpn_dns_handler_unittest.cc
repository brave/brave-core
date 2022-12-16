// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/vpn_dns_handler.h"

#include <utility>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_dns_delegate.h"
#include "brave/components/brave_vpn/browser/connection/win/utils.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

class MockVpnDnsHandler : public VpnDnsHandler, public BraveVpnDnsDelegate {
 public:
  explicit MockVpnDnsHandler(base::OnceClosure callback)
      : VpnDnsHandler(this), quit_callback_(std::move(callback)) {
    SetCloseEngineResultForTesting(true);
    SetPlatformFiltersResultForTesting(true);
  }
  ~MockVpnDnsHandler() override {}

  bool IsActive() const { return VpnDnsHandler::IsActive(); }
  void StartMonitoring() { StartVPNConnectionChangeMonitoring(); }
  void UpdateFiltersState() { VpnDnsHandler::UpdateFiltersState(); }
  void SubscribeForRasNotifications(HANDLE event_handle) override {
    event_handle_ = event_handle;
  }
  void SetConnectionResultForTesting(internal::CheckConnectionResult result) {
    VpnDnsHandler::SetConnectionResultForTesting(result);
  }
  void SetPlatformFiltersResultForTesting(bool value) {
    VpnDnsHandler::SetPlatformFiltersResultForTesting(value);
  }
  bool SetFilters(const std::wstring& connection_name) {
    return VpnDnsHandler::SetFilters(connection_name);
  }
  bool Subscribed() const { return event_handle_ != nullptr; }
  HANDLE GetEventHandle() { return event_handle_; }

  void OnObjectSignaled(HANDLE object) override {
    VpnDnsHandler::OnObjectSignaled(object);
  }

  // BraveVpnDnsDelegate
  void SignalExit() override { std::move(quit_callback_).Run(); }

 private:
  HANDLE event_handle_ = nullptr;
  base::OnceClosure quit_callback_;
};

class VpnDnsHandlerTest : public testing::Test {
 public:
  VpnDnsHandlerTest() {}
  void FastForwardBy(base::TimeDelta delta) {
    task_environment_.FastForwardBy(delta);
  }

 private:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(VpnDnsHandlerTest, Disconnected) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  EXPECT_FALSE(handler.IsActive());
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::DISCONNECTED);
  handler.StartMonitoring();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(VpnDnsHandlerTest, ConnectingFailed) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  EXPECT_FALSE(handler.IsActive());
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTING);
  handler.StartMonitoring();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(handler.IsActive());
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::DISCONNECTED);

  // Repeating interval to check the connection is live.
  // kCheckConnectionIntervalInSeconds
  FastForwardBy(base::Seconds(3));
  EXPECT_FALSE(handler.IsActive());
  // Disconnected status should stop service.
  EXPECT_TRUE(callback_called);
}

TEST_F(VpnDnsHandlerTest, ConnectingSuccessFailedToSetFilters) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  EXPECT_FALSE(handler.IsActive());
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTING);
  handler.StartMonitoring();
  base::RunLoop().RunUntilIdle();

  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTED);
  handler.SetPlatformFiltersResultForTesting(false);
  // Repeating interval to check the connection is live.
  // kCheckConnectionIntervalInSeconds
  FastForwardBy(base::Seconds(3));
  EXPECT_FALSE(handler.IsActive());
  // Failed to set filters, stop service
  EXPECT_TRUE(callback_called);
}

TEST_F(VpnDnsHandlerTest, ConnectingSuccessFiltersInstalled) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  EXPECT_FALSE(handler.IsActive());
  EXPECT_EQ(handler.GetEventHandle(), nullptr);
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTING);
  handler.StartMonitoring();
  base::RunLoop().RunUntilIdle();
  EXPECT_NE(handler.GetEventHandle(), nullptr);
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTED);
  handler.SetPlatformFiltersResultForTesting(true);
  // Repeating interval to check the connection is live.
  // kCheckConnectionIntervalInSeconds
  FastForwardBy(base::Seconds(3));
  EXPECT_TRUE(handler.IsActive());
  // Waiting for signals from RAS.
  EXPECT_FALSE(callback_called);
  EXPECT_TRUE(handler.Subscribed());

  EXPECT_NE(handler.GetEventHandle(), nullptr);

  // Disconnected.
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::DISCONNECTED);

  // BraveVpn changed state.
  handler.OnObjectSignaled(handler.GetEventHandle());
  // Vpn disconnected and service stoped.
  EXPECT_FALSE(handler.IsActive());
  EXPECT_TRUE(callback_called);
}

TEST_F(VpnDnsHandlerTest, Connected) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  EXPECT_FALSE(handler.IsActive());
  // Disconnected.
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTED);
  handler.StartMonitoring();
  base::RunLoop().RunUntilIdle();
  // Set filters immediately as vpn is connected.
  EXPECT_TRUE(handler.IsActive());
  EXPECT_FALSE(callback_called);
  EXPECT_TRUE(handler.Subscribed());

  // Disconnected.
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::DISCONNECTED);
  // BraveVpn changed state.
  handler.OnObjectSignaled(handler.GetEventHandle());
  // Vpn disconnected and service stoped.
  EXPECT_FALSE(handler.IsActive());
  EXPECT_TRUE(callback_called);
}

TEST_F(VpnDnsHandlerTest, CheckConnectedState) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  EXPECT_FALSE(handler.IsActive());
  // Connected.
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTED);
  handler.UpdateFiltersState();
  // Set filters immediately as vpn is connected.
  EXPECT_TRUE(handler.IsActive());
  EXPECT_FALSE(callback_called);
}

TEST_F(VpnDnsHandlerTest, CheckConnectingState) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  EXPECT_FALSE(handler.IsActive());
  // Connecting.
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::CONNECTING);
  handler.UpdateFiltersState();
  // Do nothing and wait until connected event.
  EXPECT_FALSE(handler.IsActive());
  EXPECT_FALSE(callback_called);
}

TEST_F(VpnDnsHandlerTest, CheckDisconnectedState) {
  bool callback_called = false;
  MockVpnDnsHandler handler(
      base::BindLambdaForTesting([&]() { callback_called = true; }));
  handler.SetPlatformFiltersResultForTesting(true);
  handler.SetFilters(std::wstring());
  EXPECT_TRUE(handler.IsActive());
  // Disconnected.
  handler.SetConnectionResultForTesting(
      internal::CheckConnectionResult::DISCONNECTED);
  handler.UpdateFiltersState();
  base::RunLoop().RunUntilIdle();
  // Remove filters
  EXPECT_FALSE(handler.IsActive());
  EXPECT_TRUE(callback_called);
}

}  // namespace brave_vpn
