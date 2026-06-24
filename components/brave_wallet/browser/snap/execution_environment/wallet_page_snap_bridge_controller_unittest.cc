/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_bridge_controller.h"

#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class WalletPageSnapBridgeControllerTest : public testing::Test {
 public:
  void SetUp() override {
    controller_ = std::make_unique<WalletPageSnapBridgeController>(
        base::BindRepeating(
            &WalletPageSnapBridgeControllerTest::OnOpenWalletPage,
            base::Unretained(this)));
  }

 protected:
  void OnOpenWalletPage() { ++open_count_; }

  base::test::TaskEnvironment task_environment_;
  FakeSnapBridge fake_bridge_;
  int open_count_ = 0;
  std::unique_ptr<WalletPageSnapBridgeController> controller_;
};

TEST_F(WalletPageSnapBridgeControllerTest, SetBridgeBinds) {
  EXPECT_FALSE(controller_->IsBound());
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());
  EXPECT_TRUE(controller_->IsBound());
}

TEST_F(WalletPageSnapBridgeControllerTest, EnsureBridgeReadyWhenBoundRunsNow) {
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());
  bool ran = false;
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran = true; }));
  EXPECT_TRUE(ran);
  EXPECT_EQ(open_count_, 0);
}

TEST_F(WalletPageSnapBridgeControllerTest, EnsureBridgeReadyWhenUnboundQueues) {
  bool ran1 = false;
  bool ran2 = false;
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran1 = true; }));
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran2 = true; }));

  // The wallet page is opened once (subsequent requests are in-flight).
  EXPECT_EQ(open_count_, 1);
  EXPECT_FALSE(ran1);
  EXPECT_FALSE(ran2);

  // Binding the bridge drains all queued callbacks.
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());
  EXPECT_TRUE(ran1);
  EXPECT_TRUE(ran2);
}

TEST_F(WalletPageSnapBridgeControllerTest, DisconnectFiresCallbackAndUnbinds) {
  bool disconnected = false;
  controller_->SetDisconnectCallback(
      base::BindLambdaForTesting([&]() { disconnected = true; }));
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());
  ASSERT_TRUE(controller_->IsBound());

  fake_bridge_.Reset();  // Close the page side of the pipe.
  task_environment_.RunUntilIdle();

  EXPECT_TRUE(disconnected);
  EXPECT_FALSE(controller_->IsBound());
}

TEST_F(WalletPageSnapBridgeControllerTest, LoadSnapPassthrough) {
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());

  base::test::TestFuture<bool, std::optional<std::string>> future;
  controller_->LoadSnap(
      "npm:test-snap",
      future.GetCallback<bool, const std::optional<std::string>&>());

  EXPECT_TRUE(future.Get<0>());
  EXPECT_EQ(fake_bridge_.load_snap_call_count, 1);
  EXPECT_EQ(fake_bridge_.last_snap_id, "npm:test-snap");
}

TEST_F(WalletPageSnapBridgeControllerTest, InvokeSnapPassthrough) {
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());

  base::test::TestFuture<std::optional<base::Value>, std::optional<std::string>>
      future;
  controller_->InvokeSnap(
      "npm:test-snap", "foo", base::Value("params"), "https://app.example.com",
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());

  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value("ok"));
  EXPECT_EQ(fake_bridge_.invoke_snap_call_count, 1);
  EXPECT_EQ(fake_bridge_.last_method, "foo");
  EXPECT_EQ(fake_bridge_.last_caller_origin, "https://app.example.com");
}

TEST_F(WalletPageSnapBridgeControllerTest, FetchSnapHomePagePassthrough) {
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());

  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>,
                         std::optional<std::string>>
      future;
  controller_->FetchSnapHomePage(
      "npm:test-snap",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&,
                         const std::optional<std::string>&>());

  EXPECT_EQ(future.Get<0>(), "{}");
  EXPECT_EQ(future.Get<1>(), "iface-1");
  EXPECT_EQ(fake_bridge_.fetch_home_page_call_count, 1);
}

TEST_F(WalletPageSnapBridgeControllerTest, SendSnapUserInputEventPassthrough) {
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());

  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>>
      future;
  controller_->SendSnapUserInputEvent(
      "npm:test-snap", "iface-1", R"({"event":"click"})",
      future.GetCallback<const std::optional<std::string>&,
                         const std::optional<std::string>&>());

  EXPECT_EQ(future.Get<0>(), "{}");
  EXPECT_EQ(fake_bridge_.user_input_call_count, 1);
  EXPECT_EQ(fake_bridge_.last_interface_id, "iface-1");
  EXPECT_EQ(fake_bridge_.last_event_json, R"({"event":"click"})");
}

}  // namespace brave_wallet
