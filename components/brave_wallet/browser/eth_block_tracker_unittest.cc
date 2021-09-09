/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_block_tracker.h"

#include <memory>
#include <string>

#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
class TrackerObserver : public EthBlockTracker::Observer {
 public:
  void OnLatestBlock(uint256_t block_num) override {
    block_num_ = block_num;
    ++observer_notified_;
  }

  size_t observer_notified_ = 0;
  uint256_t block_num_ = 0;
};
}  // namespace

class EthBlockTrackerUnitTest : public testing::Test {
 public:
  EthBlockTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  void SetUp() override {
    user_prefs::UserPrefs::Set(browser_context_.get(), &prefs_);
    EthJsonRpcController::RegisterProfilePrefs(prefs_.registry());
    rpc_controller_.reset(new brave_wallet::EthJsonRpcController(
        shared_url_loader_factory_, &prefs_));
  }
  std::string GetResponseString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
           Uint256ValueToHex(response_block_num_) + "\"}";
  }

 protected:
  uint256_t response_block_num_ = 0;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  TestingPrefServiceSimple prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<EthJsonRpcController> rpc_controller_;
};

TEST_F(EthBlockTrackerUnitTest, Timer) {
  EthBlockTracker tracker(rpc_controller_.get());
  bool request_sent = false;
  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&](const network::ResourceRequest& request) { request_sent = true; }));
  EXPECT_FALSE(tracker.IsRunning());
  tracker.Start(base::TimeDelta::FromSeconds(5));
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(1));
  EXPECT_TRUE(tracker.IsRunning());
  EXPECT_FALSE(request_sent);
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(4));
  EXPECT_TRUE(request_sent);

  // interval will be changed
  tracker.Start(base::TimeDelta::FromSeconds(30));
  request_sent = false;
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(5));
  EXPECT_FALSE(request_sent);
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(25));
  EXPECT_TRUE(request_sent);

  request_sent = false;
  tracker.Stop();
  EXPECT_FALSE(tracker.IsRunning());
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(40));
  EXPECT_FALSE(request_sent);
}

TEST_F(EthBlockTrackerUnitTest, GetBlockNumber) {
  EthBlockTracker tracker(rpc_controller_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(),
                                        GetResponseString());
      }));
  response_block_num_ = 1;
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::TimeDelta::FromSeconds(5));
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(5));
  EXPECT_EQ(observer.block_num_, uint256_t(1));
  EXPECT_EQ(observer.observer_notified_, 1u);
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(1));

  response_block_num_ = 3;
  tracker.Start(base::TimeDelta::FromSeconds(5));
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(5));
  EXPECT_EQ(observer.block_num_, uint256_t(3));
  EXPECT_EQ(observer.observer_notified_, 2u);
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(3));

  tracker.Stop();
  response_block_num_ = 4;
  // Explicity check latest block won't trigger observer nor update current
  // block
  bool callback_called = false;
  tracker.CheckForLatestBlock(
      base::BindLambdaForTesting([&](bool status, uint256_t block_num) {
        callback_called = true;
        EXPECT_TRUE(status);
        EXPECT_EQ(block_num, uint256_t(4));
      }));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.block_num_, uint256_t(3));
  EXPECT_EQ(observer.observer_notified_, 2u);
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(3));
}

TEST_F(EthBlockTrackerUnitTest, GetBlockNumberError) {
  EthBlockTracker tracker(rpc_controller_.get());
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(
            request.url.spec(), "{code: 3, message: 'Error', data: []}");
      }));

  response_block_num_ = 3;
  TrackerObserver observer;
  tracker.AddObserver(&observer);

  tracker.Start(base::TimeDelta::FromSeconds(5));
  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(5));
  EXPECT_EQ(observer.observer_notified_, 0u);
  EXPECT_EQ(observer.block_num_, uint256_t(0));
  EXPECT_EQ(tracker.GetCurrentBlock(), uint256_t(0));

  bool callback_called = false;
  tracker.CheckForLatestBlock(
      base::BindLambdaForTesting([&](bool status, uint256_t block_num) {
        callback_called = true;
        EXPECT_FALSE(status);
        EXPECT_EQ(block_num, uint256_t(0));
      }));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

}  // namespace brave_wallet
