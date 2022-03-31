/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_block_tracker.h"

#include <memory>
#include <string>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {
class TrackerObserver : public EthBlockTracker::Observer {
 public:
  void OnLatestBlock(uint256_t block_num) override {
    block_num_ = block_num;
    ++latest_block_fired_;
  }
  void OnNewBlock(uint256_t block_num) override {
    ++new_block_fired_;
    block_num_from_new_block_ = block_num;
  }

  size_t latest_block_fired_ = 0;
  size_t new_block_fired_ = 0;
  uint256_t block_num_ = 0;
  uint256_t block_num_from_new_block_ = 0;
};
}  // namespace

class EthBlockTrackerUnitTest : public testing::Test {
 public:
  EthBlockTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new brave_wallet::JsonRpcService(shared_url_loader_factory_, &prefs_));
  }
  std::string GetResponseString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
           Uint256ValueToHex(response_block_num_) + "\"}";
  }

 protected:
  uint256_t response_block_num_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
};

TEST_F(EthBlockTrackerUnitTest, Timer) {
  EthBlockTracker tracker(json_rpc_service_.get());
  bool request_sent = false;
  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&](const network::ResourceRequest& request) { request_sent = true; }));
  EXPECT_FALSE(tracker.IsRunning());
  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(1));
  EXPECT_TRUE(tracker.IsRunning());
  EXPECT_FALSE(request_sent);
  task_environment_.FastForwardBy(base::Seconds(4));
  EXPECT_TRUE(request_sent);

  // interval will be changed
  tracker.Start(base::Seconds(30));
  request_sent = false;
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_FALSE(request_sent);
  task_environment_.FastForwardBy(base::Seconds(25));
  EXPECT_TRUE(request_sent);

  request_sent = false;
  tracker.Stop();
  EXPECT_FALSE(tracker.IsRunning());
  task_environment_.FastForwardBy(base::Seconds(40));
  EXPECT_FALSE(request_sent);
}

TEST_F(EthBlockTrackerUnitTest, GetBlockNumber) {
  EthBlockTracker tracker(json_rpc_service_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_block_num_ = 1;
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.block_num_, uint256_t(1));
  EXPECT_EQ(observer.block_num_from_new_block_, uint256_t(1));
  EXPECT_EQ(observer.latest_block_fired_, 1u);
  EXPECT_EQ(observer.new_block_fired_, 1u);
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(1));

  response_block_num_ = 3;
  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.block_num_, uint256_t(3));
  EXPECT_EQ(observer.block_num_from_new_block_, uint256_t(3));
  EXPECT_EQ(observer.latest_block_fired_, 2u);
  EXPECT_EQ(observer.new_block_fired_, 2u);
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(3));

  // Still response_block_num_ 3
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.block_num_, uint256_t(3));
  EXPECT_EQ(observer.block_num_from_new_block_, uint256_t(3));
  EXPECT_EQ(observer.latest_block_fired_, 3u);
  EXPECT_EQ(observer.new_block_fired_, 2u);
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(3));

  tracker.Stop();
  response_block_num_ = 4;
  // Explicity check latest block won't trigger observer nor update current
  // block
  bool callback_called = false;
  tracker.CheckForLatestBlock(base::BindLambdaForTesting(
      [&](uint256_t block_num, brave_wallet::mojom::ProviderError error,
          const std::string& error_message) {
        callback_called = true;
        EXPECT_EQ(error, mojom::ProviderError::kSuccess);
        EXPECT_TRUE(error_message.empty());
        EXPECT_EQ(block_num, uint256_t(4));
      }));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.block_num_, uint256_t(3));
  EXPECT_EQ(observer.block_num_from_new_block_, uint256_t(3));
  EXPECT_EQ(observer.latest_block_fired_, 3u);
  EXPECT_EQ(observer.new_block_fired_, 2u);
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(3));
}

TEST_F(EthBlockTrackerUnitTest, GetBlockNumberInvalidResponseJSON) {
  EthBlockTracker tracker(json_rpc_service_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "One ring to rule them all");
      }));

  response_block_num_ = 3;
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_block_fired_, 0u);
  EXPECT_EQ(observer.block_num_, uint256_t(0));
  EXPECT_EQ(observer.block_num_from_new_block_, uint256_t(0));
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(0));

  bool callback_called = false;
  tracker.CheckForLatestBlock(base::BindLambdaForTesting(
      [&](uint256_t block_num, brave_wallet::mojom::ProviderError error,
          const std::string& error_message) {
        callback_called = true;
        EXPECT_EQ(error, mojom::ProviderError::kParsingError);
        EXPECT_EQ(error_message,
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
        EXPECT_EQ(block_num, uint256_t(0));
      }));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthBlockTrackerUnitTest, GetBlockNumberLimitExceeded) {
  EthBlockTracker tracker(json_rpc_service_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        R"({
            "jsonrpc":"2.0",
            "id":1,
            "error": {
              "code":-32005,
              "message": "Request exceeds defined limit"
            }
          })");
      }));

  response_block_num_ = 3;
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_block_fired_, 0u);
  EXPECT_EQ(observer.block_num_, uint256_t(0));
  EXPECT_EQ(observer.block_num_from_new_block_, uint256_t(0));
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(0));

  bool callback_called = false;
  tracker.CheckForLatestBlock(base::BindLambdaForTesting(
      [&](uint256_t block_num, brave_wallet::mojom::ProviderError error,
          const std::string& error_message) {
        callback_called = true;
        EXPECT_EQ(error, mojom::ProviderError::kLimitExceeded);
        EXPECT_EQ(error_message, "Request exceeds defined limit");
        EXPECT_EQ(block_num, uint256_t(0));
      }));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthBlockTrackerUnitTest, GetBlockNumberRequestTimeout) {
  EthBlockTracker tracker(json_rpc_service_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_block_num_ = 3;
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_block_fired_, 0u);
  EXPECT_EQ(observer.block_num_, uint256_t(0));
  EXPECT_EQ(observer.block_num_from_new_block_, uint256_t(0));
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(0));

  bool callback_called = false;
  tracker.CheckForLatestBlock(base::BindLambdaForTesting(
      [&](uint256_t block_num, brave_wallet::mojom::ProviderError error,
          const std::string& error_message) {
        callback_called = true;
        EXPECT_EQ(error, mojom::ProviderError::kInternalError);
        EXPECT_EQ(error_message,
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
        EXPECT_EQ(block_num, uint256_t(0));
      }));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

}  // namespace brave_wallet
