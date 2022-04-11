/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_block_tracker.h"

#include <memory>
#include <string>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

class TrackerObserver : public SolanaBlockTracker::Observer {
 public:
  void OnLatestBlockhashUpdated(const std::string& latest_blockhash,
                                uint64_t last_valid_block_height) override {
    latest_blockhash_ = latest_blockhash;
    last_valid_block_height_ = last_valid_block_height;
    ++latest_blockhash_updated_fired_;
  }

  size_t latest_blockhash_updated_fired() const {
    return latest_blockhash_updated_fired_;
  }
  std::string latest_blockhash() const { return latest_blockhash_; }
  uint64_t last_valid_block_height() const { return last_valid_block_height_; }

 private:
  size_t latest_blockhash_updated_fired_ = 0;
  std::string latest_blockhash_;
  uint64_t last_valid_block_height_ = 0;
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
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    tracker_.reset(new SolanaBlockTracker(json_rpc_service_.get()));
  }

  std::string GetResponseString() const {
    return "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
           "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":\"" +
           response_blockhash_ + "\", \"lastValidBlockHeight\":" +
           std::to_string(response_last_valid_block_height_) + "}}}";
  }

  void TestGetLatestBlockhash(bool try_cached_value,
                              const std::string& expected_latest_blockhash,
                              uint64_t expected_last_valid_block_height,
                              mojom::SolanaProviderError expected_error,
                              const std::string& expected_error_message) {
    base::RunLoop run_loop;
    tracker_->GetLatestBlockhash(
        base::BindLambdaForTesting([&](const std::string& latest_blockhash,
                                       uint64_t last_valid_block_height,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
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
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash1");
  EXPECT_EQ(observer.last_valid_block_height(), 3090u);
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 1u);
  EXPECT_EQ(tracker_->latest_blockhash(), "hash1");
  EXPECT_EQ(tracker_->last_valid_block_height(), 3090u);

  response_blockhash_ = "hash2";
  response_last_valid_block_height_ = 3290;
  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash2");
  EXPECT_EQ(observer.last_valid_block_height(), 3290u);
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 2u);
  EXPECT_EQ(tracker_->latest_blockhash(), "hash2");
  EXPECT_EQ(tracker_->last_valid_block_height(), 3290u);

  // Still response_blockhash_ hash2, shouldn't fire updated event.
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash2");
  EXPECT_EQ(observer.last_valid_block_height(), 3290u);
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 2u);
  EXPECT_EQ(tracker_->latest_blockhash(), "hash2");
  EXPECT_EQ(tracker_->last_valid_block_height(), 3290u);
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
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.last_valid_block_height(), 0u);
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_blockhash(), "");
  EXPECT_EQ(tracker_->last_valid_block_height(), 0u);
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
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.last_valid_block_height(), 0u);
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_blockhash(), "");
  EXPECT_EQ(tracker_->last_valid_block_height(), 0u);
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
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.last_valid_block_height(), 0u);
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_blockhash(), "");
  EXPECT_EQ(tracker_->last_valid_block_height(), 0u);
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashWithoutStartTracker) {
  ASSERT_FALSE(tracker_->IsRunning());

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));

  response_blockhash_ = "hash1";
  response_last_valid_block_height_ = 3090;
  TestGetLatestBlockhash(false, "hash1", 3090,
                         mojom::SolanaProviderError::kSuccess, "");

  // Cached value would be used if it's not expired yet.
  response_blockhash_ = "hash2";
  response_last_valid_block_height_ = 3290;
  TestGetLatestBlockhash(true, "hash1", 3090,
                         mojom::SolanaProviderError::kSuccess, "");
  TestGetLatestBlockhash(false, "hash2", 3290,
                         mojom::SolanaProviderError::kSuccess, "");

  // Cached value would expire after kBlockTrackerDefaultTimeInSeconds.
  response_blockhash_ = "hash3";
  response_last_valid_block_height_ = 3490;
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  TestGetLatestBlockhash(true, "hash3", 3490,
                         mojom::SolanaProviderError::kSuccess, "");
  TestGetLatestBlockhash(false, "hash3", 3490,
                         mojom::SolanaProviderError::kSuccess, "");

  // Callback should be called when JSON-RPC failed, cached blockhash won't be
  // updated and can continue to be used until expired.
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));
  TestGetLatestBlockhash(false, "", 0,
                         mojom::SolanaProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestGetLatestBlockhash(true, "hash3", 3490,
                         mojom::SolanaProviderError::kSuccess, "");
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  TestGetLatestBlockhash(false, "", 0,
                         mojom::SolanaProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestGetLatestBlockhash(true, "", 0,
                         mojom::SolanaProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

}  // namespace brave_wallet
