/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"

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

class TrackerObserver : public FilBlockTracker::Observer {
 public:
  void OnLatestBlockhashUpdated(const std::string& latest_blockhash) override {
    latest_blockhash_ = latest_blockhash;
    ++latest_blockhash_updated_fired_;
  }

  size_t latest_blockhash_updated_fired() const {
    return latest_blockhash_updated_fired_;
  }
  std::string latest_blockhash() const { return latest_blockhash_; }

 private:
  size_t latest_blockhash_updated_fired_ = 0;
  std::string latest_blockhash_;
};

}  // namespace

class FilBlockTrackerUnitTest : public testing::Test {
 public:
  FilBlockTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~FilBlockTrackerUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    tracker_.reset(new FilBlockTracker(json_rpc_service_.get()));
  }

  std::string GetResponseString() const {
    return "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"Blocks\":[],\"Cids\":[{"
           "\"/\":\"" +
           response_blockhash_ + "\"}],\"Height\": 22452}}";
  }

  void TestGetLatestBlockhash(bool try_cached_value,
                              const std::string& expected_latest_blockhash,
                              mojom::FilecoinProviderError expected_error,
                              const std::string& expected_error_message) {
    base::RunLoop run_loop;
    tracker_->GetLatestBlockhash(
        base::BindLambdaForTesting([&](const std::string& latest_blockhash,
                                       mojom::FilecoinProviderError error,
                                       const std::string& error_message) {
          EXPECT_EQ(latest_blockhash, expected_latest_blockhash);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }),
        try_cached_value);
    run_loop.Run();
  }

 protected:
  std::string response_blockhash_;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<FilBlockTracker> tracker_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(FilBlockTrackerUnitTest, GetLatestBlockhash) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_blockhash_ = "hash1";
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash1");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 1u);
  EXPECT_EQ(tracker_->latest_blockhash(), "hash1");

  response_blockhash_ = "hash2";
  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash2");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 2u);
  EXPECT_EQ(tracker_->latest_blockhash(), "hash2");

  // Still response_blockhash_ hash2, shouldn't fire updated event.
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash2");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 2u);
  EXPECT_EQ(tracker_->latest_blockhash(), "hash2");
}

TEST_F(FilBlockTrackerUnitTest, GetLatestBlockhashInvalidResponseJSON) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "May the force be with you");
      }));

  response_blockhash_ = "hash3";
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_blockhash(), "");
}

TEST_F(FilBlockTrackerUnitTest, GetLatestBlockhashInternalError) {
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
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_blockhash(), "");
}

TEST_F(FilBlockTrackerUnitTest, GetLatestBlockhashRequestTimeout) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_blockhash_ = "hash3";
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_blockhash(), "");
}

TEST_F(FilBlockTrackerUnitTest, GetLatestBlockhashWithoutStartTracker) {
  ASSERT_FALSE(tracker_->IsRunning());

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));

  response_blockhash_ = "hash1";
  TestGetLatestBlockhash(false, "hash1", mojom::FilecoinProviderError::kSuccess,
                         "");

  // Cached value would be used if it's not expired yet.
  response_blockhash_ = "hash2";
  TestGetLatestBlockhash(true, "hash1", mojom::FilecoinProviderError::kSuccess,
                         "");
  TestGetLatestBlockhash(false, "hash2", mojom::FilecoinProviderError::kSuccess,
                         "");

  // Cached value would expire after kBlockTrackerDefaultTimeInSeconds.
  response_blockhash_ = "hash3";
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  TestGetLatestBlockhash(true, "hash3", mojom::FilecoinProviderError::kSuccess,
                         "");
  TestGetLatestBlockhash(false, "hash3", mojom::FilecoinProviderError::kSuccess,
                         "");

  // Callback should be called when JSON-RPC failed, cached blockhash won't be
  // updated and can continue to be used until expired.
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));
  TestGetLatestBlockhash(false, "",
                         mojom::FilecoinProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestGetLatestBlockhash(true, "hash3", mojom::FilecoinProviderError::kSuccess,
                         "");
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  TestGetLatestBlockhash(false, "",
                         mojom::FilecoinProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  TestGetLatestBlockhash(true, "", mojom::FilecoinProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

}  // namespace brave_wallet
