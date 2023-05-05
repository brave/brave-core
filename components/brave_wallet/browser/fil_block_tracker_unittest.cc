/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
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

class MockTrackerObserver : public FilBlockTracker::Observer {
 public:
  explicit MockTrackerObserver(FilBlockTracker* tracker) {
    observation_.Observe(tracker);
  }

  MOCK_METHOD2(OnLatestHeightUpdated, void(const std::string&, uint64_t));

 private:
  base::ScopedObservation<FilBlockTracker, FilBlockTracker::Observer>
      observation_{this};
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
    json_rpc_service_ =
        std::make_unique<JsonRpcService>(shared_url_loader_factory_, &prefs_);
    tracker_ = std::make_unique<FilBlockTracker>(json_rpc_service_.get());
  }

  std::string GetResponseString() const {
    return "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"Blocks\":[],\"Cids\":[{"
           "\"/\":\"cid\"}],\"Height\":" +
           std::to_string(response_height_) + "}}";
  }

  void TestGetLatestHeight(const std::string& chain_id,
                           uint64_t expected_latest_height,
                           mojom::FilecoinProviderError expected_error,
                           const std::string& expected_error_message) {
    base::RunLoop run_loop;
    tracker_->GetFilBlockHeight(
        chain_id,
        base::BindLambdaForTesting([&](uint64_t latest_height,
                                       mojom::FilecoinProviderError error,
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
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kFilecoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kFilecoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer,
              OnLatestHeightUpdated(mojom::kFilecoinMainnet, UINT64_MAX))
      .Times(1);
  EXPECT_CALL(observer,
              OnLatestHeightUpdated(mojom::kFilecoinTestnet, UINT64_MAX))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinMainnet), UINT64_MAX);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinTestnet), UINT64_MAX);
  EXPECT_EQ(tracker_->GetLatestHeight("skynet"), 0u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  response_height_ = 1;
  tracker_->Start(mojom::kFilecoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kFilecoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(mojom::kFilecoinMainnet, 1u))
      .Times(1);
  EXPECT_CALL(observer, OnLatestHeightUpdated(mojom::kFilecoinTestnet, 1u))
      .Times(1);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinMainnet), 1u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinTestnet), 1u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  // Still response_height_ 1, shouldn't fire updated event.
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinMainnet), 1u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinTestnet), 1u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(FilBlockTrackerUnitTest, GetLatestHeightInvalidResponseJSON) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        "May the force be with you");
      }));

  response_height_ = UINT64_MAX;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kFilecoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kFilecoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinMainnet), 0u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinTestnet), 0u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
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
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kFilecoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kFilecoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinMainnet), 0u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinTestnet), 0u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(FilBlockTrackerUnitTest, GetLatestHeightRequestTimeout) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "",
                                        net::HTTP_REQUEST_TIMEOUT);
      }));

  response_height_ = 3;
  MockTrackerObserver observer(tracker_.get());

  tracker_->Start(mojom::kFilecoinMainnet, base::Seconds(5));
  tracker_->Start(mojom::kFilecoinTestnet, base::Seconds(2));
  EXPECT_CALL(observer, OnLatestHeightUpdated(_, _)).Times(0);
  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinMainnet), 0u);
  EXPECT_EQ(tracker_->GetLatestHeight(mojom::kFilecoinTestnet), 0u);
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(FilBlockTrackerUnitTest, GetLatestHeightWithoutStartTracker) {
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));

  response_height_ = 1;
  MockTrackerObserver observer(tracker_.get());
  for (const std::string& chain_id :
       {mojom::kFilecoinMainnet, mojom::kFilecoinTestnet}) {
    ASSERT_FALSE(tracker_->IsRunning(chain_id));
    EXPECT_CALL(observer, OnLatestHeightUpdated(chain_id, 1u)).Times(1);
    TestGetLatestHeight(chain_id, 1, mojom::FilecoinProviderError::kSuccess,
                        "");
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
  }
}

}  // namespace brave_wallet
