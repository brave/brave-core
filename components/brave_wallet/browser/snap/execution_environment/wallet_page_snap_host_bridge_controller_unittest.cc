/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_host_bridge_controller.h"

#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class WalletPageSnapHostBridgeControllerUnitTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  WalletPageSnapHostBridgeController controller_;
};

TEST_F(WalletPageSnapHostBridgeControllerUnitTest, NotBoundInitially) {
  EXPECT_FALSE(controller_.IsBound());
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest, BindNewBridgeBinds) {
  FakeSnapHostBridge bridge;
  controller_.BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_TRUE(controller_.IsBound());
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest,
       SecondBindNewBridgeReplacesFirst) {
  FakeSnapHostBridge bridge1;
  FakeSnapHostBridge bridge2;
  controller_.BindNewBridge(bridge1.BindNewPipeAndPassRemote());
  controller_.BindNewBridge(bridge2.BindNewPipeAndPassRemote());
  EXPECT_TRUE(controller_.IsBound());

  // The first bridge's receiver should have disconnected.
  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  controller_.LoadSnap(
      "npm:test", "module.exports='x'",
      future.GetCallback<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  auto [success, error, result] = future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(1, bridge2.load_snap_call_count);
  EXPECT_EQ(0, bridge1.load_snap_call_count);
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest, LoadSnapWhileUnbound) {
  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  controller_.LoadSnap(
      "npm:test", "src",
      future.GetCallback<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  auto [success, error, result] = future.Take();
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Snap host bridge disconnected", *error);
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest, LoadSnapPassthrough) {
  FakeSnapHostBridge bridge;
  controller_.BindNewBridge(bridge.BindNewPipeAndPassRemote());

  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  controller_.LoadSnap(
      "npm:test", "module.exports='hi'",
      future.GetCallback<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  auto [success, error, result] = future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ("npm:test", bridge.last_snap_id);
  EXPECT_EQ("module.exports='hi'", bridge.last_source_code);
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest,
       UnloadSnapWhileUnboundIsNoop) {
  controller_.UnloadSnap("npm:test");  // Should not crash.
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest, DisconnectUnbinds) {
  FakeSnapHostBridge bridge;
  controller_.BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_TRUE(controller_.IsBound());

  bridge.Reset();

  // Disconnection is detected asynchronously; LoadSnap's reply only arrives
  // once the controller's remote has processed the error and reset itself.
  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  controller_.LoadSnap(
      "npm:test", "src",
      future.GetCallback<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  auto [success, error, result] = future.Take();
  EXPECT_FALSE(success);
  EXPECT_FALSE(controller_.IsBound());
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest,
       EnsureBridgeReadyRunsSynchronously) {
  bool ran = false;
  controller_.EnsureBridgeReady(
      base::BindOnce([](bool* ran) { *ran = true; }, &ran));
  EXPECT_TRUE(ran);
}

TEST_F(WalletPageSnapHostBridgeControllerUnitTest, ShutdownUnbinds) {
  FakeSnapHostBridge bridge;
  controller_.BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_TRUE(controller_.IsBound());

  controller_.Shutdown();
  EXPECT_FALSE(controller_.IsBound());
}

}  // namespace brave_wallet
