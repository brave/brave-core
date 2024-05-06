/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_block_tracker.h"

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
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

class MockTrackerObserver : public SolanaBlockTracker::Observer {
 public:
  explicit MockTrackerObserver(SolanaBlockTracker* tracker) {
    observation_.Observe(tracker);
  }

  MOCK_METHOD3(OnLatestBlockhashUpdated,
               void(const std::string&, const std::string&, uint64_t));

 private:
  base::ScopedObservation<SolanaBlockTracker, SolanaBlockTracker::Observer>
      observation_{this};
};

}  // namespace

class SolanaBlockTrackerUnitTest : public testing::Test {
 public:
  SolanaBlockTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~SolanaBlockTrackerUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_ =
        std::make_unique<JsonRpcService>(shared_url_loader_factory_, &prefs_);
    tracker_ = std::make_unique<SolanaBlockTracker>(json_rpc_service_.get());
  }

  std::string GetResponseString() const {
    return "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
           "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":\"" +
           response_blockhash_ + "\", \"lastValidBlockHeight\":" +
           std::to_string(response_last_valid_block_height_) + "}}}";
  }

  void TestGetLatestBlockhash(const base::Location& location,
                              const std::string& chain_id,
                              bool try_cached_value,
                              const std::string& expected_latest_blockhash,
                              uint64_t expected_last_valid_block_height,
                              mojom::SolanaProviderError expected_error,
                              const std::string& expected_error_message) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    tracker_->GetLatestBlockhash(
        chain_id,
        base::BindLambdaForTesting([&](const std::string& latest_blockhash,
                                       uint64_t last_valid_block_height,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(error_message, expected_error_message);
          ASSERT_EQ(error, expected_error);
          EXPECT_EQ(latest_blockhash, expected_latest_blockhash);
          EXPECT_EQ(last_valid_block_height, expected_last_valid_block_height);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }),
        try_cached_value);
    run_loop.Run();
  }

 protected:
  std::string response_blockhash_;
  uint64_t response_last_valid_block_height_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<SolanaBlockTracker> tracker_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhash) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_blockhash_ = "hash1";
  response_last_valid_block_height_ = 3090;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kSolanaMainnet, base::Seconds(5));
  tracker_->Start(mojom::kSolanaTestnet, base::Seconds(2));
  EXPECT_CALL(observer,
              OnLatestBlockhashUpdated(mojom::kSolanaMainnet, "hash1", 3090u))
      .Times(1);
  EXPECT_CALL(observer,
              OnLatestBlockhashUpdated(mojom::kSolanaTestnet, "hash1", 3090u))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  response_blockhash_ = "hash2";
  response_last_valid_block_height_ = 3290;
  tracker_->Start(mojom::kSolanaMainnet, base::Seconds(5));
  tracker_->Start(mojom::kSolanaTestnet, base::Seconds(2));
  EXPECT_CALL(observer,
              OnLatestBlockhashUpdated(mojom::kSolanaMainnet, "hash2", 3290u))
      .Times(1);
  EXPECT_CALL(observer,
              OnLatestBlockhashUpdated(mojom::kSolanaTestnet, "hash2", 3290u))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  // Still response_blockhash_ hash2, shouldn't fire updated event.
  EXPECT_CALL(observer, OnLatestBlockhashUpdated(_, _, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashInvalidResponseJSON) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "May the force be with you");
      }));

  response_blockhash_ = "hash3";
  response_last_valid_block_height_ = 3290;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kSolanaMainnet, base::Seconds(5));
  tracker_->Start(mojom::kSolanaTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlockhashUpdated(_, _, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashInternalError) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        R"({
            "jsonrpc":"2.0",
            "id":1,
            "error": {
              "code":-32603,
              "message": "Internal JSON RPC error"
            }
          })");
      }));

  response_blockhash_ = "hash3";
  response_last_valid_block_height_ = 3290;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kSolanaMainnet, base::Seconds(5));
  tracker_->Start(mojom::kSolanaTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlockhashUpdated(_, _, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashRequestTimeout) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_blockhash_ = "hash3";
  response_last_valid_block_height_ = 3290;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kSolanaMainnet, base::Seconds(5));
  tracker_->Start(mojom::kSolanaTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestBlockhashUpdated(_, _, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashWithoutStartTracker) {
  MockTrackerObserver observer(tracker_.get());
  for (const std::string& chain_id :
       {mojom::kSolanaMainnet, mojom::kSolanaTestnet}) {
    SCOPED_TRACE(chain_id);
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(),
                                          GetResponseString());
        }));

    ASSERT_FALSE(tracker_->IsRunning(chain_id));

    response_blockhash_ = "hash1";
    response_last_valid_block_height_ = 3090;
    EXPECT_CALL(observer, OnLatestBlockhashUpdated(chain_id, "hash1", 3090u))
        .Times(1);
    TestGetLatestBlockhash(FROM_HERE, chain_id, false, "hash1", 3090,
                           mojom::SolanaProviderError::kSuccess, "");
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

    // Cached value would be used if it's not expired yet.
    response_blockhash_ = "hash2";
    response_last_valid_block_height_ = 3290;
    EXPECT_CALL(observer, OnLatestBlockhashUpdated(_, _, _)).Times(0);
    TestGetLatestBlockhash(FROM_HERE, chain_id, true, "hash1", 3090,
                           mojom::SolanaProviderError::kSuccess, "");
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

    EXPECT_CALL(observer, OnLatestBlockhashUpdated(chain_id, "hash2", 3290u))
        .Times(1);
    TestGetLatestBlockhash(FROM_HERE, chain_id, false, "hash2", 3290,
                           mojom::SolanaProviderError::kSuccess, "");
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

    // Cached value would expire after kSolanaBlockTrackerTimeInSeconds.
    response_blockhash_ = "hash3";
    response_last_valid_block_height_ = 3490;
    task_environment_.FastForwardBy(
        base::Seconds(kSolanaBlockTrackerTimeInSeconds));
    EXPECT_CALL(observer, OnLatestBlockhashUpdated(chain_id, "hash3", 3490u))
        .Times(1);
    TestGetLatestBlockhash(FROM_HERE, chain_id, true, "hash3", 3490,
                           mojom::SolanaProviderError::kSuccess, "");
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
    EXPECT_CALL(observer, OnLatestBlockhashUpdated(_, _, _)).Times(0);
    TestGetLatestBlockhash(FROM_HERE, chain_id, true, "hash3", 3490,
                           mojom::SolanaProviderError::kSuccess, "");
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

    // Callback should be called when JSON-RPC failed, cached blockhash won't be
    // updated and can continue to be used until expired.
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
    EXPECT_CALL(observer, OnLatestBlockhashUpdated(_, _, _)).Times(0);
    TestGetLatestBlockhash(FROM_HERE, chain_id, false, "", 0,
                           mojom::SolanaProviderError::kInternalError,
                           l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    TestGetLatestBlockhash(FROM_HERE, chain_id, true, "hash3", 3490,
                           mojom::SolanaProviderError::kSuccess, "");
    task_environment_.FastForwardBy(
        base::Seconds(kSolanaBlockTrackerTimeInSeconds));
    TestGetLatestBlockhash(FROM_HERE, chain_id, false, "", 0,
                           mojom::SolanaProviderError::kInternalError,
                           l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    TestGetLatestBlockhash(FROM_HERE, chain_id, true, "", 0,
                           mojom::SolanaProviderError::kInternalError,
                           l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  }
}

}  // namespace brave_wallet
