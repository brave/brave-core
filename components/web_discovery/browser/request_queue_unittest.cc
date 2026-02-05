/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/request_queue.h"

#include <memory>
#include <utility>

#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/web_discovery_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

namespace {
constexpr auto kWaitInterval = base::Minutes(2);
}

class WebDiscoveryRequestQueueTest : public testing::Test {
 public:
  WebDiscoveryRequestQueueTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~WebDiscoveryRequestQueueTest() override = default;

  void SetUp() override {
    WebDiscoveryService::RegisterProfilePrefs(prefs_.registry());
    queue_ = std::make_unique<RequestQueue>(
        &prefs_, kScheduledReports,
        /*request_max_age=*/base::Minutes(5),
        /*min_request_interval=*/base::Seconds(60),
        /*max_request_interval=*/base::Seconds(90),
        /*max_retries=*/3,
        base::BindRepeating(&WebDiscoveryRequestQueueTest::OnStartRequest,
                            base::Unretained(this)));
  }

 protected:
  void OnStartRequest(const base::Value& request) {
    last_request_ = request.Clone();
    requests_made_++;
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<RequestQueue> queue_;
  size_t requests_made_ = 0;
  base::Value last_request_;
};

TEST_F(WebDiscoveryRequestQueueTest, BasicRequest) {
  base::DictValue request;
  request.Set("test", "value");
  queue_->ScheduleRequest(std::move(request));
  EXPECT_EQ(requests_made_, 0u);

  task_environment_.FastForwardBy(base::Seconds(20));
  EXPECT_EQ(requests_made_, 0u);

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);
  ASSERT_TRUE(last_request_.is_dict());
  EXPECT_EQ(*last_request_.GetDict().FindString("test"), "value");

  queue_->NotifyRequestComplete(true);
  requests_made_ = 0;

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 0u);
}

TEST_F(WebDiscoveryRequestQueueTest, RetryRequest) {
  base::DictValue request;
  request.Set("test", "value");
  queue_->ScheduleRequest(std::move(request));

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);

  queue_->NotifyRequestComplete(false);
  requests_made_ = 0;

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);
  ASSERT_TRUE(last_request_.is_dict());
  EXPECT_EQ(*last_request_.GetDict().FindString("test"), "value");

  queue_->NotifyRequestComplete(true);
  requests_made_ = 0;

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 0u);
}

TEST_F(WebDiscoveryRequestQueueTest, MaxRetriesExceeded) {
  base::DictValue request;
  request.Set("test", "value");
  queue_->ScheduleRequest(std::move(request));

  // First try
  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);
  queue_->NotifyRequestComplete(false);
  requests_made_ = 0;

  // Second try
  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);
  queue_->NotifyRequestComplete(false);
  requests_made_ = 0;

  // Third try
  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);
  queue_->NotifyRequestComplete(false);
  requests_made_ = 0;

  // Request should be dropped
  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 0u);
}

TEST_F(WebDiscoveryRequestQueueTest, MultipleRequests) {
  base::DictValue request1;
  request1.Set("test", "value1");
  queue_->ScheduleRequest(std::move(request1));

  base::DictValue request2;
  request2.Set("test", "value2");
  queue_->ScheduleRequest(std::move(request2));

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);
  ASSERT_TRUE(last_request_.is_dict());
  EXPECT_EQ(*last_request_.GetDict().FindString("test"), "value1");

  queue_->NotifyRequestComplete(true);
  requests_made_ = 0;

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 1u);
  ASSERT_TRUE(last_request_.is_dict());
  EXPECT_EQ(*last_request_.GetDict().FindString("test"), "value2");

  queue_->NotifyRequestComplete(true);
  requests_made_ = 0;

  task_environment_.FastForwardBy(kWaitInterval);
  EXPECT_EQ(requests_made_, 0u);
}

}  // namespace web_discovery
