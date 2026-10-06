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
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_wallet {

class HiddenWebContentsSnapHostBridgeControllerUnitTest
    : public content::RenderViewHostTestHarness {
 protected:
  void TearDown() override {
    // The controller's hidden WebContents must be destroyed before the
    // harness tears down its RenderProcessHost/RenderWidgetHost bookkeeping.
    controller_.reset();
    content::RenderViewHostTestHarness::TearDown();
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
  int ready_count = 0;
  auto queue_ready = [&ready_count, this] {
    controller_->EnsureBridgeReady(
        base::BindOnce([](int* c) { ++*c; }, &ready_count));
  };
  queue_ready();
  queue_ready();
  queue_ready();

  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);
  // None run until the page binds, and the stacked calls share one host.
  EXPECT_EQ(0, ready_count);
  EXPECT_FALSE(controller_->IsBound());
  EXPECT_EQ(host, controller_->host_for_testing());

  FakeSnapHostBridge bridge;
  controller_->BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_EQ(3, ready_count);
  EXPECT_TRUE(controller_->IsBound());
  EXPECT_EQ(host, controller_->host_for_testing());

  // Already bound: runs now, instead of joining another queue.
  queue_ready();
  EXPECT_EQ(4, ready_count);
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       InFlightHostReloadsOnlyWhenCrashed) {
  CreateController();
  controller_->EnsureBridgeReady(base::DoNothing());
  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);
  ASSERT_FALSE(host->IsCrashed());
  auto* pending = host->GetController().GetPendingEntry();
  ASSERT_NE(nullptr, pending);
  const int pending_entry_id = pending->GetUniqueID();

  // Start is in flight and the renderer is alive: do not navigate again.
  controller_->EnsureBridgeReady(base::DoNothing());
  EXPECT_EQ(host, controller_->host_for_testing());
  ASSERT_NE(nullptr, host->GetController().GetPendingEntry());
  EXPECT_EQ(pending_entry_id,
            host->GetController().GetPendingEntry()->GetUniqueID());

  static_cast<content::MockRenderProcessHost*>(
      host->GetPrimaryMainFrame()->GetProcess())
      ->SimulateCrash();
  ASSERT_TRUE(host->IsCrashed());
  EXPECT_EQ(host, controller_->host_for_testing());
  auto* crashed_pending = host->GetController().GetPendingEntry();
  const int crashed_entry_id =
      crashed_pending ? crashed_pending->GetUniqueID() : -1;

  // No pipe was bound, so the crash does not disconnect. Reload this host.
  controller_->EnsureBridgeReady(base::DoNothing());
  EXPECT_EQ(host, controller_->host_for_testing());
  auto* reloaded = host->GetController().GetPendingEntry();
  ASSERT_NE(nullptr, reloaded);
  EXPECT_NE(crashed_entry_id, reloaded->GetUniqueID());
}

// A load that ends in an error page will never bind, so queued callers must
// be released rather than left waiting for a BindNewBridge() that can't come.
TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       FailedLoadRunsPendingCallbacks) {
  CreateController();
  int ready_count = 0;
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);
  EXPECT_EQ(0, ready_count);

  auto navigation =
      content::NavigationSimulator::CreateFromPending(host->GetController());
  navigation->Fail(net::ERR_FAILED);
  navigation->CommitErrorPage();

  EXPECT_EQ(1, ready_count);
  EXPECT_FALSE(controller_->IsBound());
}

// A successful commit is not yet a bound bridge: the page binds later, once
// its script runs. Callers must keep waiting instead of failing here.
TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       CommittedLoadKeepsCallbacksPending) {
  CreateController();
  int ready_count = 0;
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);

  content::NavigationSimulator::CreateFromPending(host->GetController())
      ->Commit();
  EXPECT_EQ(0, ready_count);

  FakeSnapHostBridge bridge;
  controller_->BindNewBridge(bridge.BindNewPipeAndPassRemote());
  EXPECT_EQ(1, ready_count);
}

// A crash before the page binds leaves no pipe, so the remote's disconnect
// handler never runs. Queued callers must still be released — each holds a
// live mojo responder that would otherwise never reply.
TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       CrashBeforeBindRunsPendingCallbacks) {
  CreateController();
  int ready_count = 0;
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  auto* host = controller_->host_for_testing();
  ASSERT_NE(nullptr, host);
  EXPECT_EQ(0, ready_count);

  static_cast<content::MockRenderProcessHost*>(
      host->GetPrimaryMainFrame()->GetProcess())
      ->SimulateCrash();

  EXPECT_EQ(2, ready_count);
  EXPECT_FALSE(controller_->IsBound());
  // The host is kept so the next EnsureBridgeReady reloads it in place.
  EXPECT_EQ(host, controller_->host_for_testing());
}

TEST_F(HiddenWebContentsSnapHostBridgeControllerUnitTest,
       ShutdownDestroysHostAndRunsPendingCallbacks) {
  CreateController();
  int ready_count = 0;
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  EXPECT_EQ(0, ready_count);
  EXPECT_NE(nullptr, controller_->host_for_testing());

  controller_->Shutdown();
  EXPECT_EQ(2, ready_count);
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

  int ready_count = 0;
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  controller_->EnsureBridgeReady(
      base::BindOnce([](int* c) { ++*c; }, &ready_count));
  EXPECT_EQ(0, ready_count);
  EXPECT_EQ(host, controller_->host_for_testing());

  FakeSnapHostBridge rebound;
  controller_->BindNewBridge(rebound.BindNewPipeAndPassRemote());
  EXPECT_EQ(2, ready_count);
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
