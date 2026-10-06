/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap_service.h"

#include <memory>
#include <optional>
#include <string>
#include <tuple>

#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
constexpr char kTestSnapId[] = "npm:test-snap";
constexpr char kTestSourceCode[] = "module.exports = 'npm:test-snap'";
}  // namespace

class SnapServiceUnitTest : public testing::Test {
 public:
  SnapServiceUnitTest() = default;

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterLocalStatePrefsForMigration(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &prefs_, &local_state_);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);

    auto fake_controller = std::make_unique<FakeSnapHostBridgeController>();
    fake_controller_ = fake_controller.get();
    service_ = std::make_unique<SnapService>(*keyring_service_,
                                             std::move(fake_controller));
    service_->SetSnapBundleForTesting(kTestSnapId, kTestSourceCode);
  }

  std::tuple<bool, std::optional<std::string>, std::optional<std::string>>
  LoadSnap(const std::string& snap_id) {
    base::test::TestFuture<bool, const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    service_->LoadSnap(
        snap_id, future.GetCallback<bool, const std::optional<std::string>&,
                                    const std::optional<std::string>&>());
    return future.Take();
  }

  void CreateWallet() {
    base::test::TestFuture<const std::optional<std::string>&> future;
    keyring_service_->CreateWallet(
        "brave1234", future.GetCallback<const std::optional<std::string>&>());
    ASSERT_TRUE(future.Wait());
  }

  // Locked()/WalletReset() reach the service over a mojo observer pipe, so
  // the shutdown is not observable until that delivery has run.
  [[nodiscard]] bool WaitForShutdown() {
    return base::test::RunUntil(
        [&] { return fake_controller_->shutdown_count == 1; });
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<SnapService> service_;
  raw_ptr<FakeSnapHostBridgeController> fake_controller_ = nullptr;
};

TEST_F(SnapServiceUnitTest, LockedKeyringFailsFast) {
  CreateWallet();
  keyring_service_->Lock();
  ASSERT_TRUE(keyring_service_->IsLockedSync());

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet is locked", *error);
  EXPECT_EQ(0, fake_controller_->ensure_bridge_ready_count);
}

TEST_F(SnapServiceUnitTest, UnknownBundleReturnsError) {
  CreateWallet();
  ASSERT_FALSE(keyring_service_->IsLockedSync());

  auto [success, error, result] = LoadSnap("npm:missing");
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Bundle not found", *error);
  EXPECT_EQ(0, fake_controller_->ensure_bridge_ready_count);
}

TEST_F(SnapServiceUnitTest, EnsureBridgeReadyCompletesUnbound) {
  CreateWallet();

  fake_controller_->set_bound(false);
  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Snap host unavailable", *error);
}

TEST_F(SnapServiceUnitTest, SuccessPathReturnsFakeResult) {
  CreateWallet();

  fake_controller_->set_bound(true);
  fake_controller_->load_snap_result = kTestSnapId;
  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success);
  EXPECT_FALSE(error.has_value());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

TEST_F(SnapServiceUnitTest, LockedRunsShutdown) {
  CreateWallet();

  keyring_service_->Lock();
  EXPECT_TRUE(WaitForShutdown());
}

TEST_F(SnapServiceUnitTest, WalletResetRunsShutdown) {
  CreateWallet();

  keyring_service_->Reset(/*notify_observer=*/true);
  EXPECT_TRUE(WaitForShutdown());
}

TEST_F(SnapServiceUnitTest, ShutdownRunsOnce) {
  service_->Shutdown();
  EXPECT_EQ(1, fake_controller_->shutdown_count);
}

TEST_F(SnapServiceUnitTest, DeferredReadyStillRunsCallbackAfterShutdown) {
  CreateWallet();

  fake_controller_->set_defer_ready(true);
  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  service_->LoadSnap(kTestSnapId,
                     future.GetCallback<bool, const std::optional<std::string>&,
                                        const std::optional<std::string>&>());
  EXPECT_FALSE(future.IsReady());

  service_->Shutdown();

  auto [success, error, result] = future.Take();
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Snap host unavailable", *error);
}

}  // namespace brave_wallet
