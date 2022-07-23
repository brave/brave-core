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
  void OnLatestHeightUpdated(uint64_t latest_height) override {
    latest_height_ = latest_height;
    ++latest_height_updated_fired_;
  }

  size_t latest_height_updated_fired() const {
    return latest_height_updated_fired_;
  }
  uint64_t latest_height() const { return latest_height_; }

 private:
  size_t latest_height_updated_fired_ = 0;
  uint64_t latest_height_ = 0;
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
           "\"/\":\"cid\"}],\"Height\":" +
           std::to_string(response_height_) + "}}";
  }

  void TestGetLatestHeight(uint64_t expected_latest_height,
                           mojom::FilecoinProviderError expected_error,
                           const std::string& expected_error_message) {
    base::RunLoop run_loop;
    tracker_->GetFilBlockHeight(base::BindLambdaForTesting(
        [&](uint64_t latest_height, mojom::FilecoinProviderError error,
            const std::string& error_message) {
          EXPECT_EQ(latest_height, expected_latest_height);
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

 protected:
  uint64_t response_height_ = 0;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<FilBlockTracker> tracker_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(FilBlockTrackerUnitTest, GetLatestHeight) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_height_ = UINT64_MAX;
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_height(), UINT64_MAX);
  EXPECT_EQ(observer.latest_height_updated_fired(), 1u);
  EXPECT_EQ(tracker_->latest_height(), UINT64_MAX);

  response_height_ = 1;
  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_height(), 1u);
  EXPECT_EQ(observer.latest_height_updated_fired(), 2u);
  EXPECT_EQ(tracker_->latest_height(), 1u);

  // Still response_height_ 1, shouldn't fire updated event.
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_height(), 1u);
  EXPECT_EQ(observer.latest_height_updated_fired(), 2u);
  EXPECT_EQ(tracker_->latest_height(), 1u);
}

TEST_F(FilBlockTrackerUnitTest, GetLatestHeightInvalidResponseJSON) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "May the force be with you");
      }));

  response_height_ = UINT64_MAX;
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_height(), 0u);
  EXPECT_EQ(observer.latest_height_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_height(), 0u);
}

TEST_F(FilBlockTrackerUnitTest, GetLatestHeightInternalError) {
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

  response_height_ = 3;
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_height(), 0u);
  EXPECT_EQ(observer.latest_height_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_height(), 0u);
}

TEST_F(FilBlockTrackerUnitTest, GetLatestHeightRequestTimeout) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_height_ = 3;
  TrackerObserver observer;
  tracker_->AddObserver(&observer);

  tracker_->Start(base::Seconds(5));
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(observer.latest_height(), 0u);
  EXPECT_EQ(observer.latest_height_updated_fired(), 0u);
  EXPECT_EQ(tracker_->latest_height(), 0u);
}

TEST_F(FilBlockTrackerUnitTest, GetLatestHeightWithoutStartTracker) {
  ASSERT_FALSE(tracker_->IsRunning());

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));

  response_height_ = 1;
  TestGetLatestHeight(1, mojom::FilecoinProviderError::kSuccess, "");
}

}  // namespace brave_wallet
