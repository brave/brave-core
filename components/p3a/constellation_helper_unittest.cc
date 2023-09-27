// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/constellation_helper.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_message.h"
#include "brave/components/p3a/star_randomness_test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr uint8_t kTestEpoch = 5;
constexpr char kTestNextEpochTime[] = "2086-06-24T18:00:00Z";
constexpr char kTestHistogramName[] = "Brave.Test.Histogram";
constexpr char kTestHost[] = "https://localhost:8443";

}  // namespace

class P3AConstellationHelperTest : public testing::Test {
 public:
  P3AConstellationHelperTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

 protected:
  void SetUp() override {
    p3a_config_.disable_star_attestation = true;
    p3a_config_.star_randomness_host = kTestHost;

    ConstellationHelper::RegisterPrefs(local_state_.registry());

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          std::string response;
          if (request.url == GURL(std::string(kTestHost) + "/info")) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kGetMethod);

            response = "{\"currentEpoch\":" + base::NumberToString(kTestEpoch) +
                       ", \"nextEpochTime\": \"" + kTestNextEpochTime + "\"}";
            info_request_made_ = true;
          } else if (request.url ==
                     GURL(std::string(kTestHost) + "/randomness")) {
            response = HandleRandomnessRequest(request, kTestEpoch);
            points_request_made_ = true;
          }
          if (!response.empty()) {
            if (interceptor_send_bad_response_) {
              response = "this is a bad response that is not json";
            }
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  void SetUpHelper() {
    server_info_from_callback_ = nullptr;

    helper_ = std::make_unique<ConstellationHelper>(
        &local_state_, shared_url_loader_factory_,
        base::BindLambdaForTesting(
            [this](std::string histogram_name, uint8_t epoch,
                   std::unique_ptr<std::string> serialized_message) {
              histogram_name_from_callback_ = histogram_name;
              epoch_from_callback_ = epoch;
              serialized_message_from_callback_ = std::move(serialized_message);
            }),
        base::BindLambdaForTesting([this](RandomnessServerInfo* server_info) {
          server_info_from_callback_ = server_info;
        }),
        &p3a_config_);
    task_environment_.RunUntilIdle();
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  P3AConfig p3a_config_;
  std::unique_ptr<ConstellationHelper> helper_;
  TestingPrefServiceSimple local_state_;
  bool interceptor_send_bad_response_ = false;

  raw_ptr<RandomnessServerInfo> server_info_from_callback_ = nullptr;
  std::unique_ptr<std::string> serialized_message_from_callback_;

  std::string histogram_name_from_callback_;
  uint8_t epoch_from_callback_;

  bool info_request_made_ = false;
  bool points_request_made_ = false;
};

TEST_F(P3AConstellationHelperTest, CanRetrieveServerInfo) {
  base::Time exp_next_epoch_time;
  ASSERT_TRUE(base::Time::FromString(kTestNextEpochTime, &exp_next_epoch_time));

  SetUpHelper();
  helper_->UpdateRandomnessServerInfo();
  task_environment_.RunUntilIdle();

  ASSERT_TRUE(info_request_made_);

  EXPECT_NE(server_info_from_callback_, nullptr);
  EXPECT_EQ(server_info_from_callback_->current_epoch, kTestEpoch);

  EXPECT_EQ(server_info_from_callback_->next_epoch_time, exp_next_epoch_time);

  // See if cached server info is used on next execution
  info_request_made_ = false;
  SetUpHelper();
  helper_->UpdateRandomnessServerInfo();
  task_environment_.RunUntilIdle();

  ASSERT_FALSE(info_request_made_);
  EXPECT_NE(server_info_from_callback_, nullptr);
  EXPECT_EQ(server_info_from_callback_->current_epoch, kTestEpoch);
  EXPECT_EQ(server_info_from_callback_->next_epoch_time, exp_next_epoch_time);
}

TEST_F(P3AConstellationHelperTest, CannotRetrieveServerInfo) {
  base::Time exp_next_epoch_time;
  ASSERT_TRUE(base::Time::FromString(kTestNextEpochTime, &exp_next_epoch_time));

  interceptor_send_bad_response_ = true;

  SetUpHelper();
  helper_->UpdateRandomnessServerInfo();
  task_environment_.RunUntilIdle();

  ASSERT_TRUE(info_request_made_);

  // callback should not be executed if info retrieval failed
  EXPECT_EQ(server_info_from_callback_, nullptr);

  // See if info retrieval retry is scheduled
  info_request_made_ = false;
  task_environment_.FastForwardBy(base::Seconds(6));

  ASSERT_TRUE(info_request_made_);
  EXPECT_EQ(server_info_from_callback_, nullptr);
}

TEST_F(P3AConstellationHelperTest, GenerateBasicMessage) {
  SetUpHelper();
  helper_->UpdateRandomnessServerInfo();
  task_environment_.RunUntilIdle();

  MessageMetainfo meta_info;
  meta_info.Init(&local_state_, "release", "2022-01-01");

  helper_->StartMessagePreparation(
      kTestHistogramName, GenerateP3AConstellationMessage(
                              kTestHistogramName, kTestEpoch, meta_info));
  task_environment_.RunUntilIdle();

  ASSERT_TRUE(points_request_made_);

  EXPECT_NE(serialized_message_from_callback_, nullptr);
  EXPECT_NE(serialized_message_from_callback_->size(), 0U);

  EXPECT_EQ(histogram_name_from_callback_, kTestHistogramName);
  EXPECT_EQ(epoch_from_callback_, kTestEpoch);
}

}  // namespace p3a
