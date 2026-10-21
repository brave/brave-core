/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/hidden_web_contents_snap_host_bridge_controller.h"

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_helpers.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_test_utils.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_wallet {

class HiddenWebContentsSnapHostBridgeControllerUnitTest
    : public ChromeRenderViewHostTestHarness {
 protected:
  void TearDown() override {
    // The controller's hidden WebContents must be destroyed before the
    // harness tears down its RenderProcessHost/RenderWidgetHost bookkeeping.
    controller_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  void CreateController() {
    controller_ = std::make_unique<HiddenWebContentsSnapHostBridgeController>(
        browser_context(), GURL(kBraveUISnapsContainerURL));
  }

  std::unique_ptr<HiddenWebContentsSnapHostBridgeController> controller_;
};

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest, NotBoundInitially) {
  CreateController();
  EXPECT_FALSE(controller_->IsBound());
  EXPECT_EQ(nullptr, controller_->host_for_testing());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       StackedEnsureBridgeReadyWaitsForSingleBind) {
  CreateController();
  base::test::TestFuture<void> ready1;
  base::test::TestFuture<void> ready2;
  base::test::TestFuture<void> ready3;
  controller_->EnsureBridgeReady(ready1.GetCallback());
  controller_->EnsureBridgeReady(ready2.GetCallback());
  controller_->EnsureBridgeReady(ready3.GetCallback());

  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);
  // None run until the page binds, and the stacked calls share one host.
  EXPECT_FALSE(ready1.IsReady());
  EXPECT_FALSE(ready2.IsReady());
  EXPECT_FALSE(ready3.IsReady());
  EXPECT_FALSE(controller_->IsBound());
  EXPECT_EQ(host, controller_->host_for_testing());

  FakeSnapHostBridge bridge;
  controller_->BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_TRUE(ready1.Wait());
  EXPECT_TRUE(ready2.Wait());
  EXPECT_TRUE(ready3.Wait());
  EXPECT_TRUE(controller_->IsBound());
  EXPECT_EQ(host, controller_->host_for_testing());

  // Already bound: runs now, instead of joining another queue.
  base::test::TestFuture<void> ready4;
  controller_->EnsureBridgeReady(ready4.GetCallback());
  EXPECT_TRUE(ready4.Wait());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       InFlightHostIsNotReloaded) {
  CreateController();
  controller_->EnsureBridgeReady(base::DoNothing());
  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);
  auto* pending = host->GetController().GetPendingEntry();
  ASSERT_NE(nullptr, pending);
  const int pending_entry_id = pending->GetUniqueID();

  controller_->EnsureBridgeReady(base::DoNothing());
  EXPECT_EQ(host, controller_->host_for_testing());
  ASSERT_NE(nullptr, host->GetController().GetPendingEntry());
  EXPECT_EQ(pending_entry_id,
            host->GetController().GetPendingEntry()->GetUniqueID());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       ShutdownDestroysHostAndRunsPendingCallbacks) {
  CreateController();
  base::test::TestFuture<void> ready1;
  base::test::TestFuture<void> ready2;
  controller_->EnsureBridgeReady(ready1.GetCallback());
  controller_->EnsureBridgeReady(ready2.GetCallback());
  EXPECT_FALSE(ready1.IsReady());
  EXPECT_FALSE(ready2.IsReady());
  EXPECT_NE(nullptr, controller_->host_for_testing());

  controller_->Shutdown();
  EXPECT_TRUE(ready1.Wait());
  EXPECT_TRUE(ready2.Wait());
  EXPECT_EQ(nullptr, controller_->host_for_testing());
  EXPECT_FALSE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       DisconnectKeepsHostAndDoesNotRecreateIt) {
  CreateController();
  controller_->EnsureBridgeReady(base::DoNothing());
  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);

  FakeSnapHostBridge bridge;
  controller_->BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_TRUE(controller_->IsBound());

  bridge.Reset();

  // Disconnection is detected asynchronously; LoadSnap's reply only arrives
  // once the controller's remote has processed the error and reset itself.
  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      disconnect_future;
  controller_->LoadSnap(
      "npm:test", "src",
      disconnect_future.GetCallback<bool, const std::optional<std::string>&,
                                    const std::optional<std::string>&>());
  auto [success, error, result] = disconnect_future.Take();
  EXPECT_FALSE(success);
  EXPECT_FALSE(controller_->IsBound());
  EXPECT_EQ(host, controller_->host_for_testing());

  base::test::TestFuture<void> ready1;
  base::test::TestFuture<void> ready2;
  controller_->EnsureBridgeReady(ready1.GetCallback());
  controller_->EnsureBridgeReady(ready2.GetCallback());
  EXPECT_FALSE(ready1.IsReady());
  EXPECT_FALSE(ready2.IsReady());
  EXPECT_EQ(host, controller_->host_for_testing());

  FakeSnapHostBridge rebound;
  controller_->BindNewBridge(rebound.BindNewPipeAndPassRemote());
  EXPECT_TRUE(ready1.Wait());
  EXPECT_TRUE(ready2.Wait());
  EXPECT_EQ(host, controller_->host_for_testing());
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
  // UnloadSnap is one-way. A following LoadSnap is ordered after it on the
  // same pipe, so its reply means the unload has been dispatched.
  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      unload_flushed;
  controller_->LoadSnap(
      "npm:test", "module.exports='hi'",
      unload_flushed.GetCallback<bool, const std::optional<std::string>&,
                                 const std::optional<std::string>&>());
  auto [flushed, flushed_error, flushed_result] = unload_flushed.Take();
  EXPECT_TRUE(flushed);
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
