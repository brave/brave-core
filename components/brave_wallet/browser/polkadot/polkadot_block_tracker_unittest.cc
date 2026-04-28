/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"

#include <memory>
#include <optional>
#include <string>

#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet {

namespace {

class MockTrackerObserver : public PolkadotBlockTracker::Observer {
 public:
  explicit MockTrackerObserver(PolkadotBlockTracker* tracker) {
    observation_.Observe(tracker);
  }

  MOCK_METHOD2(OnLatestBlock, void(const std::string&, uint32_t));

 private:
  base::ScopedObservation<PolkadotBlockTracker, PolkadotBlockTracker::Observer>
      observation_{this};
};

}  // namespace

class PolkadotBlockTrackerUnitTest : public testing::Test {
 public:
  PolkadotBlockTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~PolkadotBlockTrackerUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);

    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());

    polkadot_mock_rpc_ = std::make_unique<PolkadotMockRpc>(
        &url_loader_factory_, network_manager_.get());

    tracker_ = std::make_unique<PolkadotBlockTracker>(*polkadot_substrate_rpc_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
  std::unique_ptr<PolkadotMockRpc> polkadot_mock_rpc_;
  std::unique_ptr<PolkadotBlockTracker> tracker_;
};

TEST_F(PolkadotBlockTrackerUnitTest, GetLatestBlock) {
  // Test that we successfully notify the observers when a new finalized
  // blockhash is presented.

  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kPolkadotMainnet, base::Seconds(5));
  tracker_->Start(mojom::kPolkadotTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlock(mojom::kPolkadotTestnet, 0x1c06355))
      .Times(1);
  EXPECT_CALL(observer, OnLatestBlock(mojom::kPolkadotMainnet, 0x1c06355))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  EXPECT_CALL(observer, OnLatestBlock(mojom::kPolkadotTestnet, 0x1c06355))
      .Times(0);
  EXPECT_CALL(observer, OnLatestBlock(mojom::kPolkadotMainnet, 0x1c06355))
      .Times(0);
  task_environment_.FastForwardBy(base::Seconds(2 * 5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(PolkadotBlockTrackerUnitTest, GetLatestBlockInvalidResponseJSON) {
  // Test that observers aren't notified if an intermediate network request
  // fails.

  polkadot_mock_rpc_->UseInvalidFinalizedBlockHash();
  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kPolkadotMainnet, base::Seconds(5));
  tracker_->Start(mojom::kPolkadotTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlock(mojom::kPolkadotTestnet, 0x1c06355))
      .Times(0);
  EXPECT_CALL(observer, OnLatestBlock(mojom::kPolkadotMainnet, 0x1c06355))
      .Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

}  // namespace brave_wallet
