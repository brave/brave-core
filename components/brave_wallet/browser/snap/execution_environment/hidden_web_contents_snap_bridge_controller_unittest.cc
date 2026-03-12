/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/execution_environment/hidden_web_contents_snap_bridge_controller.h"

#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_renderer_host.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

// Logic-only coverage: the foreground-debug feature is enabled so
// EnsureHostStarted opens a (fake) debug tab instead of creating a real hidden
// WebContents that would load chrome://wallet-snap-host/. The live host /
// DidFinishLoad flow is intended for a browser test.
class HiddenWebContentsSnapBridgeControllerTest
    : public content::RenderViewHostTestHarness {
 public:
  HiddenWebContentsSnapBridgeControllerTest() {
    scoped_feature_list_.InitAndEnableFeature(
        features::kBraveWalletSnapsBackgroundForegroundDebug);
  }

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterLocalStatePrefsForMigration(local_state_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &prefs_, nullptr);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);
  }

  void TearDown() override {
    controller_.reset();
    keyring_service_.reset();
    json_rpc_service_.reset();
    network_manager_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

 protected:
  void CreateController() {
    controller_ = std::make_unique<HiddenWebContentsSnapBridgeController>(
        *keyring_service_, browser_context(),
        base::BindRepeating(
            &HiddenWebContentsSnapBridgeControllerTest::OnOpenDebugTab,
            base::Unretained(this)));
  }

  void Unlock() {
    AccountUtils(keyring_service_.get())
        .CreateWallet(kMnemonicDivideCruise, "brave123");
  }

  void OnOpenDebugTab() { ++debug_tab_count_; }

  base::test::ScopedFeatureList scoped_feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  TestingPrefServiceSimple local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  FakeSnapBridge fake_bridge_;
  int debug_tab_count_ = 0;
  std::unique_ptr<HiddenWebContentsSnapBridgeController> controller_;
};

TEST_F(HiddenWebContentsSnapBridgeControllerTest, IsBoundFalseInitially) {
  CreateController();
  EXPECT_FALSE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest,
       EnsureBridgeReadyWhenLockedRunsImmediately) {
  // A wallet-less keyring reports unlocked; create then lock it so the
  // fail-fast (locked) branch is exercised.
  Unlock();
  keyring_service_->Lock();
  ASSERT_TRUE(keyring_service_->IsLockedSync());
  CreateController();

  bool ran = false;
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran = true; }));
  EXPECT_TRUE(ran);
  EXPECT_EQ(debug_tab_count_, 0);  // Fail-fast; host is not started.
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest,
       EnsureBridgeReadyWhenBoundRunsImmediately) {
  CreateController();
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());
  bool ran = false;
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran = true; }));
  EXPECT_TRUE(ran);
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest,
       UnlockedQueuesOpensDebugTabAndDrains) {
  Unlock();
  CreateController();

  bool ran1 = false;
  bool ran2 = false;
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran1 = true; }));
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran2 = true; }));

  EXPECT_EQ(debug_tab_count_, 1);  // Opened once; the second call is in-flight.
  EXPECT_FALSE(ran1);
  EXPECT_FALSE(ran2);

  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());
  EXPECT_TRUE(ran1);
  EXPECT_TRUE(ran2);
  EXPECT_TRUE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest,
       LockedTearsDownAndFailsPending) {
  Unlock();
  CreateController();
  bool ran = false;
  bool disconnected = false;
  controller_->SetDisconnectCallback(
      base::BindLambdaForTesting([&]() { disconnected = true; }));
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran = true; }));
  ASSERT_FALSE(ran);  // Queued while the host starts.

  controller_->Locked();

  EXPECT_TRUE(ran);  // Pending callback flushed on teardown.
  EXPECT_TRUE(disconnected);
  EXPECT_FALSE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest, WalletResetTearsDown) {
  Unlock();
  CreateController();
  bool ran = false;
  controller_->EnsureBridgeReady(
      base::BindLambdaForTesting([&]() { ran = true; }));
  ASSERT_FALSE(ran);

  controller_->WalletReset();
  EXPECT_TRUE(ran);
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest, DisconnectViaPipeUnbinds) {
  CreateController();
  bool disconnected = false;
  controller_->SetDisconnectCallback(
      base::BindLambdaForTesting([&]() { disconnected = true; }));
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());
  ASSERT_TRUE(controller_->IsBound());

  fake_bridge_.Reset();
  task_environment()->RunUntilIdle();

  EXPECT_TRUE(disconnected);
  EXPECT_FALSE(controller_->IsBound());
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest, LoadSnapPassthrough) {
  CreateController();
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());

  base::test::TestFuture<bool, std::optional<std::string>> future;
  controller_->LoadSnap(
      "npm:test-snap",
      future.GetCallback<bool, const std::optional<std::string>&>());

  EXPECT_TRUE(future.Get<0>());
  EXPECT_EQ(fake_bridge_.load_snap_call_count, 1);
  EXPECT_EQ(fake_bridge_.last_snap_id, "npm:test-snap");
}

TEST_F(HiddenWebContentsSnapBridgeControllerTest, InvokeSnapPassthrough) {
  CreateController();
  controller_->SetBridge(fake_bridge_.BindNewPipeAndPassRemote());

  base::test::TestFuture<std::optional<base::Value>, std::optional<std::string>>
      future;
  controller_->InvokeSnap(
      "npm:test-snap", "foo", base::Value(), "brave-wallet",
      future.GetCallback<std::optional<base::Value>,
                         const std::optional<std::string>&>());

  ASSERT_TRUE(future.Get<0>());
  EXPECT_EQ(*future.Get<0>(), base::Value("ok"));
  EXPECT_EQ(fake_bridge_.invoke_snap_call_count, 1);
}

}  // namespace brave_wallet
