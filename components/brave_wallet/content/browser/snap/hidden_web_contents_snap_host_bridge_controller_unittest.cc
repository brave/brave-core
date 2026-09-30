/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/content/browser/snap/hidden_web_contents_snap_host_bridge_controller.h"

#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_test_utils.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
constexpr char kHostUrl[] = "chrome://wallet-snap-host/";
}  // namespace

class HiddenWebContentsSnapHostBridgeControllerUnitTest
    : public content::RenderViewHostTestHarness {
 protected:
  void TearDown() override {
    // The controller's hidden WebContents must be destroyed before the
    // harness tears down its RenderProcessHost/RenderWidgetHost bookkeeping.
    controller_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  void CreateController(bool start_host = true) {
    controller_ = std::make_unique<HiddenWebContentsSnapHostBridgeController>(
        browser_context(), GURL(kHostUrl), start_host);
  }

  int CallEnsureBridgeReady() {
    int call_count = 0;
    controller_->EnsureBridgeReady(
        base::BindOnce([](int* count) { ++*count; }, &call_count));
    return call_count;
  }

  std::unique_ptr<HiddenWebContentsSnapHostBridgeController> controller_;
};

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest, NotBoundInitially) {
  CreateController();
  EXPECT_FALSE(controller_->IsBound());
  EXPECT_EQ(nullptr, controller_->host_for_testing());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       EnsureBridgeReadyCreatesOneHost) {
  CreateController();
  controller_->EnsureBridgeReady(base::DoNothing());
  EXPECT_NE(nullptr, controller_->host_for_testing());
  auto* host = controller_->host_for_testing();

  // A second concurrent call should not create another host.
  controller_->EnsureBridgeReady(base::DoNothing());
  EXPECT_EQ(host, controller_->host_for_testing());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       BindNewBridgeDrainsAllQueuedCallbacks) {
  CreateController();
  int ready_count = 0;
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));

  FakeSnapHostBridge bridge;
  controller_->BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_EQ(2, ready_count);
  EXPECT_TRUE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       ShutdownDestroysHostAndRunsPendingCallbacks) {
  CreateController();
  int ready_count = 0;
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  EXPECT_NE(nullptr, controller_->host_for_testing());

  controller_->Shutdown();
  EXPECT_EQ(1, ready_count);
  EXPECT_EQ(nullptr, controller_->host_for_testing());
  EXPECT_FALSE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       DisconnectTearsDownAndAllowsRestart) {
  CreateController();
  FakeSnapHostBridge bridge;
  controller_->EnsureBridgeReady(base::DoNothing());
  controller_->BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_TRUE(controller_->IsBound());

  bridge.Reset();
  task_environment()->RunUntilIdle();
  EXPECT_FALSE(controller_->IsBound());
  EXPECT_EQ(nullptr, controller_->host_for_testing());

  // Next EnsureBridgeReady should start a new host.
  controller_->EnsureBridgeReady(base::DoNothing());
  EXPECT_NE(nullptr, controller_->host_for_testing());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       StartHostFalseQueuesWithoutCreatingHost) {
  CreateController(/*start_host=*/false);
  controller_->EnsureBridgeReady(base::DoNothing());
  EXPECT_EQ(nullptr, controller_->host_for_testing());
  EXPECT_FALSE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       LoadUnloadSnapPassthrough) {
  CreateController();
  FakeSnapHostBridge bridge;
  controller_->BindNewBridge(bridge.BindNewPipeAndPassRemote());

  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  controller_->LoadSnap(
      "npm:test", "module.exports='hi'",
      future.GetCallback<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  auto [success, error, result] = future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(1, bridge.load_snap_call_count);

  controller_->UnloadSnap("npm:test");
  task_environment()->RunUntilIdle();
  EXPECT_EQ(1, bridge.unload_snap_call_count);
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       LoadSnapWhileUnboundReturnsDisconnectError) {
  CreateController();
  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  controller_->LoadSnap(
      "npm:test", "src",
      future.GetCallback<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>());
  auto [success, error, result] = future.Take();
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Snap host bridge disconnected", *error);
}

}  // namespace brave_wallet
