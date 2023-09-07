/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_block_tracker.h"

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using testing::_;

namespace brave_wallet {

namespace {

class MockTrackerObserver : public EthBlockTracker::Observer {
 public:
  explicit MockTrackerObserver(EthBlockTracker* tracker) {
    observation_.Observe(tracker);
  }

  MOCK_METHOD2(OnLatestBlock, void(const std::string&, uint256_t));
  MOCK_METHOD2(OnNewBlock, void(const std::string&, uint256_t));

 private:
  base::ScopedObservation<EthBlockTracker, EthBlockTracker::Observer>
      observation_{this};
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
    json_rpc_service_ = std::make_unique<brave_wallet::JsonRpcService>(
        shared_url_loader_factory_, &prefs_);
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
  EXPECT_FALSE(tracker.IsRunning(mojom::kMainnetChainId));
  tracker.Start(mojom::kMainnetChainId, base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(1));
  EXPECT_TRUE(tracker.IsRunning(mojom::kMainnetChainId));
  EXPECT_FALSE(request_sent);
  task_environment_.FastForwardBy(base::Seconds(4));
  EXPECT_TRUE(request_sent);
  EXPECT_FALSE(tracker.IsRunning(mojom::kGoerliChainId));

  // interval will be changed
  tracker.Start(mojom::kMainnetChainId, base::Seconds(30));
  request_sent = false;
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_FALSE(request_sent);
  task_environment_.FastForwardBy(base::Seconds(25));
  EXPECT_TRUE(request_sent);
  EXPECT_FALSE(tracker.IsRunning(mojom::kGoerliChainId));

  tracker.Stop(mojom::kMainnetChainId);
  EXPECT_FALSE(tracker.IsRunning(mojom::kMainnetChainId));
  tracker.Start(mojom::kMainnetChainId, base::Seconds(5));
  EXPECT_TRUE(tracker.IsRunning(mojom::kMainnetChainId));

  request_sent = false;
  tracker.Start(mojom::kGoerliChainId, base::Seconds(10));
  EXPECT_TRUE(tracker.IsRunning(mojom::kGoerliChainId));
  tracker.Stop();
  EXPECT_FALSE(tracker.IsRunning(mojom::kMainnetChainId));
  EXPECT_FALSE(tracker.IsRunning(mojom::kGoerliChainId));
  task_environment_.FastForwardBy(base::Seconds(10));
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
  MockTrackerObserver observer(&tracker);

  tracker.Start(mojom::kMainnetChainId, base::Seconds(5));
  tracker.Start(mojom::kGoerliChainId, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlock(mojom::kMainnetChainId, 1)).Times(1);
  EXPECT_CALL(observer, OnLatestBlock(mojom::kGoerliChainId, 1)).Times(2);
  EXPECT_CALL(observer, OnNewBlock(mojom::kMainnetChainId, 1)).Times(1);
  EXPECT_CALL(observer, OnNewBlock(mojom::kGoerliChainId, 1)).Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kMainnetChainId), uint256_t(1));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kGoerliChainId), uint256_t(1));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  response_block_num_ = 3;
  tracker.Start(mojom::kMainnetChainId, base::Seconds(5));
  tracker.Start(mojom::kGoerliChainId, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlock(mojom::kMainnetChainId, 3)).Times(1);
  EXPECT_CALL(observer, OnLatestBlock(mojom::kGoerliChainId, 3)).Times(2);
  EXPECT_CALL(observer, OnNewBlock(mojom::kMainnetChainId, 3)).Times(1);
  EXPECT_CALL(observer, OnNewBlock(mojom::kGoerliChainId, 3)).Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kMainnetChainId), uint256_t(3));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kGoerliChainId), uint256_t(3));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  // Still response_block_num_ 3
  EXPECT_CALL(observer, OnLatestBlock(mojom::kMainnetChainId, 3)).Times(1);
  EXPECT_CALL(observer, OnLatestBlock(mojom::kGoerliChainId, 3)).Times(3);
  EXPECT_CALL(observer, OnNewBlock(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kMainnetChainId), uint256_t(3));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kGoerliChainId), uint256_t(3));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  tracker.Stop();
  response_block_num_ = 4;
  // Explicity check latest block won't trigger observer nor update current
  // block
  for (const std::string& chain_id :
       {mojom::kMainnetChainId, mojom::kGoerliChainId}) {
    SCOPED_TRACE(chain_id);
    EXPECT_CALL(observer, OnLatestBlock(_, _)).Times(0);
    EXPECT_CALL(observer, OnNewBlock(_, _)).Times(0);
    base::RunLoop run_loop;
    tracker.CheckForLatestBlock(
        chain_id,
        base::BindLambdaForTesting([&](uint256_t block_num,
                                       brave_wallet::mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(error, mojom::ProviderError::kSuccess);
          EXPECT_TRUE(error_message.empty());
          EXPECT_EQ(block_num, uint256_t(4));
          run_loop.Quit();
        }));
    run_loop.Run();
    EXPECT_EQ(tracker.GetCurrentBlock(chain_id), uint256_t(3));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  }
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
  MockTrackerObserver observer(&tracker);

  tracker.Start(mojom::kMainnetChainId, base::Seconds(5));
  tracker.Start(mojom::kGoerliChainId, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlock(_, _)).Times(0);
  EXPECT_CALL(observer, OnNewBlock(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kMainnetChainId), uint256_t(0));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kGoerliChainId), uint256_t(0));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  for (const std::string& chain_id :
       {mojom::kMainnetChainId, mojom::kGoerliChainId}) {
    SCOPED_TRACE(chain_id);
    base::RunLoop run_loop;
    tracker.CheckForLatestBlock(
        chain_id,
        base::BindLambdaForTesting([&](uint256_t block_num,
                                       brave_wallet::mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(error, mojom::ProviderError::kParsingError);
          EXPECT_EQ(error_message,
                    l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
          EXPECT_EQ(block_num, uint256_t(0));
          run_loop.Quit();
        }));
    run_loop.Run();
  }
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
  MockTrackerObserver observer(&tracker);

  tracker.Start(mojom::kMainnetChainId, base::Seconds(5));
  tracker.Start(mojom::kGoerliChainId, base::Seconds(2));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_CALL(observer, OnLatestBlock(testing::_, testing::_)).Times(0);
  EXPECT_CALL(observer, OnNewBlock(testing::_, testing::_)).Times(0);
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kMainnetChainId), uint256_t(0));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kGoerliChainId), uint256_t(0));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  for (const std::string& chain_id :
       {mojom::kMainnetChainId, mojom::kGoerliChainId}) {
    SCOPED_TRACE(chain_id);
    base::RunLoop run_loop;
    tracker.CheckForLatestBlock(
        chain_id,
        base::BindLambdaForTesting([&](uint256_t block_num,
                                       brave_wallet::mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(error, mojom::ProviderError::kLimitExceeded);
          EXPECT_EQ(error_message, "Request exceeds defined limit");
          EXPECT_EQ(block_num, uint256_t(0));
          run_loop.Quit();
        }));
    run_loop.Run();
  }
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
  MockTrackerObserver observer(&tracker);

  tracker.Start(mojom::kMainnetChainId, base::Seconds(5));
  tracker.Start(mojom::kGoerliChainId, base::Seconds(2));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_CALL(observer, OnLatestBlock(testing::_, testing::_)).Times(0);
  EXPECT_CALL(observer, OnNewBlock(testing::_, testing::_)).Times(0);
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kMainnetChainId), uint256_t(0));
  EXPECT_EQ(tracker.GetCurrentBlock(mojom::kGoerliChainId), uint256_t(0));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  for (const std::string& chain_id :
       {mojom::kMainnetChainId, mojom::kGoerliChainId}) {
    SCOPED_TRACE(chain_id);
    base::RunLoop run_loop;
    tracker.CheckForLatestBlock(
        chain_id,
        base::BindLambdaForTesting([&](uint256_t block_num,
                                       brave_wallet::mojom::ProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(error, mojom::ProviderError::kInternalError);
          EXPECT_EQ(error_message,
                    l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
          EXPECT_EQ(block_num, uint256_t(0));
          run_loop.Quit();
        }));
    run_loop.Run();
  }
}

}  // namespace brave_wallet
