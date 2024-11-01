/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_block_tracker.h"

#include <memory>
#include <optional>
#include <string>

#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
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

class MockTrackerObserver : public BitcoinBlockTracker::Observer {
 public:
  explicit MockTrackerObserver(BitcoinBlockTracker* tracker) {
    observation_.Observe(tracker);
  }

  MOCK_METHOD2(OnLatestHeightUpdated, void(const std::string&, uint32_t));

 private:
  base::ScopedObservation<BitcoinBlockTracker, BitcoinBlockTracker::Observer>
      observation_{this};
};

}  // namespace

class BitcoinBlockTrackerUnitTest : public testing::Test {
 public:
  BitcoinBlockTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~BitcoinBlockTrackerUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    bitcoin_rpc_ = std::make_unique<bitcoin_rpc::BitcoinRpc>(
        *network_manager_, shared_url_loader_factory_);
    tracker_ = std::make_unique<BitcoinBlockTracker>(*bitcoin_rpc_);
  }

  std::string GetResponseString() const {
    return base::NumberToString(response_height_);
  }

 protected:
  uint32_t response_height_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<bitcoin_rpc::BitcoinRpc> bitcoin_rpc_;
  std::unique_ptr<BitcoinBlockTracker> tracker_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinBlockTrackerUnitTest, GetLatestHeight) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_height_ = UINT32_MAX;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kBitcoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kBitcoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer,
              OnLatestHeightUpdated(mojom::kBitcoinMainnet, UINT32_MAX))
      .Times(1);
  EXPECT_CALL(observer,
              OnLatestHeightUpdated(mojom::kBitcoinTestnet, UINT32_MAX))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinMainnet), UINT32_MAX);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinTestnet), UINT32_MAX);
  EXPECT_EQ(tracker_->GetLatestHeight("skynet"), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  response_height_ = 1;
  tracker_->Start(mojom::kBitcoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kBitcoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(mojom::kBitcoinMainnet, 1u))
      .Times(1);
  EXPECT_CALL(observer, OnLatestHeightUpdated(mojom::kBitcoinTestnet, 1u))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinMainnet), 1u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinTestnet), 1u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  // Still response_height_ 1, shouldn't fire updated event.
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinMainnet), 1u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinTestnet), 1u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(BitcoinBlockTrackerUnitTest, GetLatestHeightInvalidResponseJSON) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "May the force be with you");
      }));

  response_height_ = UINT32_MAX;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kBitcoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kBitcoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinMainnet), std::nullopt);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinTestnet), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(BitcoinBlockTrackerUnitTest, GetLatestHeightInternalError) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "Error",
                                        net::HTTP_INTERNAL_SERVER_ERROR);
      }));

  response_height_ = 3;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kBitcoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kBitcoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinMainnet), std::nullopt);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinTestnet), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(BitcoinBlockTrackerUnitTest, GetLatestHeightRequestTimeout) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_height_ = 3;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kBitcoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kBitcoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinMainnet), std::nullopt);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kBitcoinTestnet), std::nullopt);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

}  // namespace brave_wallet
