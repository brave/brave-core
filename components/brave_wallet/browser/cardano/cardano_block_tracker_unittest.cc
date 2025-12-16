/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_block_tracker.h"

#include <memory>
#include <optional>
#include <string>

#include "base/json/json_writer.h"
#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet {

namespace {

class MockTrackerObserver : public CardanoBlockTracker::Observer {
 public:
  explicit MockTrackerObserver(CardanoBlockTracker* tracker) {
    observation_.Observe(tracker);
  }

  MOCK_METHOD2(OnLatestHeightUpdated, void(const std::string&, uint32_t));

 private:
  base::ScopedObservation<CardanoBlockTracker, CardanoBlockTracker::Observer>
      observation_{this};
};

}  // namespace

class CardanoBlockTrackerUnitTest : public testing::Test {
 public:
  CardanoBlockTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~CardanoBlockTrackerUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    cardano_wallet_service_ = std::make_unique<CardanoWalletService>(
        *keyring_service_, *network_manager_, shared_url_loader_factory_);
    tracker_ = std::make_unique<CardanoBlockTracker>(*cardano_wallet_service_);
  }

  std::string GetResponseString() const {
    cardano_rpc::blockfrost_api::Block block;
    block.epoch = "123";
    block.slot = "123";
    block.height = base::NumberToString(response_height_);

    return *base::WriteJson(block.ToValue());
  }

 protected:
  uint32_t response_height_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<CardanoWalletService> cardano_wallet_service_;
  std::unique_ptr<CardanoBlockTracker> tracker_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(CardanoBlockTrackerUnitTest, GetLatestHeight) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_height_ = UINT32_MAX;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kCardanoMainnet, base::Seconds(5));
  tracker_->Start(mojom::kCardanoTestnet, base::Seconds(2));
  EXPECT_CALL(observer,
              OnLatestHeightUpdated(mojom::kCardanoMainnet, UINT32_MAX))
      .Times(1);
  EXPECT_CALL(observer,
              OnLatestHeightUpdated(mojom::kCardanoTestnet, UINT32_MAX))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoMainnet), UINT32_MAX);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoTestnet), UINT32_MAX);
  EXPECT_EQ(tracker_->GetLatestHeight("skynet"), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  response_height_ = 1;
  tracker_->Start(mojom::kCardanoMainnet, base::Seconds(5));
  tracker_->Start(mojom::kCardanoTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(mojom::kCardanoMainnet, 1u))
      .Times(1);
  EXPECT_CALL(observer, OnLatestHeightUpdated(mojom::kCardanoTestnet, 1u))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoMainnet), 1u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoTestnet), 1u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  // Still response_height_ 1, shouldn't fire updated event.
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoMainnet), 1u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoTestnet), 1u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(CardanoBlockTrackerUnitTest, GetLatestHeightInvalidResponseJSON) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "May the force be with you");
      }));

  response_height_ = UINT32_MAX;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kCardanoMainnet, base::Seconds(5));
  tracker_->Start(mojom::kCardanoTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoMainnet), std::nullopt);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoTestnet), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(CardanoBlockTrackerUnitTest, GetLatestHeightInternalError) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "Error",
                                        net::HTTP_INTERNAL_SERVER_ERROR);
      }));

  response_height_ = 3;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kCardanoMainnet, base::Seconds(5));
  tracker_->Start(mojom::kCardanoTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoMainnet), std::nullopt);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoTestnet), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(CardanoBlockTrackerUnitTest, GetLatestHeightRequestTimeout) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_height_ = 3;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kCardanoMainnet, base::Seconds(5));
  tracker_->Start(mojom::kCardanoTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoMainnet), std::nullopt);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kCardanoTestnet), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

}  // namespace brave_wallet
