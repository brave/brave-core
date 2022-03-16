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
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

class TrackerObserver : public SolanaBlockTracker::Observer {
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
        new brave_wallet::JsonRpcService(shared_url_loader_factory_, &prefs_));
  }

  std::string GetResponseString() const {
    return "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
           "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":\"" +
           response_blockhash_ + "\", \"lastValidBlockHeight\":3090}}}";
  }

 protected:
  std::string response_blockhash_;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
};

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhash) {
  SolanaBlockTracker tracker(json_rpc_service_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_blockhash_ = "hash1";
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash1");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 1u);
  EXPECT_EQ(tracker.latest_blockhash(), "hash1");

  response_blockhash_ = "hash2";
  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash2");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 2u);
  EXPECT_EQ(tracker.latest_blockhash(), "hash2");

  // Still response_blockhash_ hash2, shouldn't fire updated event.
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "hash2");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 2u);
  EXPECT_EQ(tracker.latest_blockhash(), "hash2");
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashInvalidResponseJSON) {
  SolanaBlockTracker tracker(json_rpc_service_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "May the force be with you");
      }));

  response_blockhash_ = "hash3";
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker.latest_blockhash(), "");
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashInternalError) {
  SolanaBlockTracker tracker(json_rpc_service_.get());
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
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker.latest_blockhash(), "");
}

TEST_F(SolanaBlockTrackerUnitTest, GetLatestBlockhashRequestTimeout) {
  SolanaBlockTracker tracker(json_rpc_service_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_blockhash_ = "hash3";
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_blockhash(), "");
  EXPECT_EQ(observer.latest_blockhash_updated_fired(), 0u);
  EXPECT_EQ(tracker.latest_blockhash(), "");
}

}  // namespace brave_wallet
