// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/brave_p3a_star.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/p3a_message.h"
#include "brave/components/p3a/star_randomness_test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

namespace {

constexpr uint8_t kTestEpoch = 5;
constexpr char kTestNextEpochTime[] = "2086-06-24T18:00:00Z";
constexpr char kTestHistogramName[] = "Brave.Test.Histogram";
constexpr char kTestHost[] = "https://localhost:8443";

}  // namespace

class P3AStarTest : public testing::Test {
 public:
  P3AStarTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory)) {}

 protected:
  void SetUp() override {
    p3a_config.disable_star_attestation = true;
    p3a_config.star_randomness_host = kTestHost;

    BraveP3AStar::RegisterPrefs(local_state.registry());
    RegisterPrefsForBraveReferralsService(local_state.registry());

    url_loader_factory.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory.ClearResponses();

          if (request.url == GURL(std::string(kTestHost) + "/info")) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kGetMethod);
            url_loader_factory.AddResponse(
                request.url.spec(),
                "{\"currentEpoch\":" + base::NumberToString(kTestEpoch) +
                    ", \"nextEpochTime\": \"" + kTestNextEpochTime + "\"}");
            info_request_made = true;
          } else if (request.url ==
                     GURL(std::string(kTestHost) + "/randomness")) {
            std::string resp_json =
                HandleRandomnessRequest(request, kTestEpoch);
            url_loader_factory.AddResponse(request.url.spec(), resp_json);

            points_request_made = true;
          }
        }));
  }

  void SetUpStarManager() {
    star_manager.reset(new BraveP3AStar(
        &local_state, shared_url_loader_factory,
        base::BindLambdaForTesting(
            [this](std::string histogram_name, uint8_t epoch,
                   std::unique_ptr<std::string> serialized_message) {
              histogram_name_from_callback = histogram_name;
              epoch_from_callback = epoch;
              serialized_message_from_callback = std::move(serialized_message);
            }),
        base::BindLambdaForTesting([this](RandomnessServerInfo* server_info) {
          server_info_from_callback = server_info;
        }),
        &p3a_config));
    task_environment_.RunUntilIdle();
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory;
  BraveP3AConfig p3a_config;
  std::unique_ptr<BraveP3AStar> star_manager;
  TestingPrefServiceSimple local_state;

  RandomnessServerInfo* server_info_from_callback;
  std::unique_ptr<std::string> serialized_message_from_callback;

  std::string histogram_name_from_callback;
  uint8_t epoch_from_callback;

  bool info_request_made;
  bool points_request_made;
};

TEST_F(P3AStarTest, CanRetrieveServerInfo) {
  base::Time exp_next_epoch_time;
  ASSERT_TRUE(base::Time::FromString(kTestNextEpochTime, &exp_next_epoch_time));

  SetUpStarManager();
  task_environment_.RunUntilIdle();

  ASSERT_TRUE(info_request_made);

  EXPECT_NE(server_info_from_callback, nullptr);
  EXPECT_EQ(server_info_from_callback->current_epoch, kTestEpoch);

  EXPECT_EQ(server_info_from_callback->next_epoch_time, exp_next_epoch_time);

  // See if cached server info is used on next execution
  info_request_made = false;
  SetUpStarManager();

  ASSERT_FALSE(info_request_made);
  EXPECT_NE(server_info_from_callback, nullptr);
  EXPECT_EQ(server_info_from_callback->current_epoch, kTestEpoch);
  EXPECT_EQ(server_info_from_callback->next_epoch_time, exp_next_epoch_time);
}

TEST_F(P3AStarTest, GenerateBasicMessage) {
  SetUpStarManager();

  MessageMetainfo meta_info;
  meta_info.Init(&local_state, "release", "2022-01-01");

  star_manager->StartMessagePreparation(
      kTestHistogramName,
      GenerateP3AStarMessage(kTestHistogramName, kTestEpoch, meta_info));
  task_environment_.RunUntilIdle();

  ASSERT_TRUE(points_request_made);

  EXPECT_NE(serialized_message_from_callback, nullptr);
  EXPECT_NE(serialized_message_from_callback->size(), 0U);

  EXPECT_EQ(histogram_name_from_callback, kTestHistogramName);
  EXPECT_EQ(epoch_from_callback, kTestEpoch);
}

}  // namespace brave
