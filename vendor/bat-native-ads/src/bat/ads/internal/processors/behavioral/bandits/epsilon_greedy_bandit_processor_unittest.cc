/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/container_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arms.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace {

constexpr char kArmsWithEmptySegmentJson[] = R"(
  {
    "travel":{"pulls":0,"segment":"travel","value":1.0},
    "":{"pulls":0,"segment":"","value":1.0}
  }
)";

}  // namespace

namespace ads {

class BatAdsEpsilonGreedyBanditProcessorTest : public UnitTestBase {
 protected:
  BatAdsEpsilonGreedyBanditProcessorTest() = default;

  ~BatAdsEpsilonGreedyBanditProcessorTest() override = default;
};

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, InitializeAllArmsFromResource) {
  // Arrange
  targeting::EpsilonGreedyBanditArmMap prefs_arms;
  targeting::EpsilonGreedyBanditArmInfo prefs_arm_info;
  prefs_arm_info.segment = "foo";
  prefs_arms["foo"] = prefs_arm_info;
  prefs_arm_info.segment = "bar";
  prefs_arms["bar"] = prefs_arm_info;

  AdsClientHelper::Get()->SetStringPref(
      prefs::kEpsilonGreedyBanditArms,
      targeting::EpsilonGreedyBanditArms::ToJson(prefs_arms));

  // Act
  processor::EpsilonGreedyBandit processor;

  // Assert
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);
  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  EXPECT_EQ(30U, arms.size());

  EXPECT_EQ(0u, arms.count("foo"));
  EXPECT_EQ(0u, arms.count("bar"));
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, NeverProcessed) {
  // Arrange
  processor::EpsilonGreedyBandit processor;

  // Act
  // rewards: [] => value: 1.0
  std::string segment = "travel";

  // Assert
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);
  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  auto iter = arms.find(segment);
  targeting::EpsilonGreedyBanditArmInfo arm = iter->second;
  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 1.0;
  expected_arm.pulls = 0;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithOneReward) {
  // Arrange
  processor::EpsilonGreedyBandit processor;

  // Act
  // rewards: [0, 0, 0, 0] => value: 0.0
  std::string segment = "travel";
  processor.Process({segment, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment, mojom::NotificationAdEventType::kTimedOut});
  processor.Process({segment, mojom::NotificationAdEventType::kDismissed});

  // Assert
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);
  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  auto iter = arms.find(segment);
  targeting::EpsilonGreedyBanditArmInfo arm = iter->second;
  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 0.0;
  expected_arm.pulls = 4;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithTwoRewards) {
  // Arrange
  processor::EpsilonGreedyBandit processor;

  // Act
  // rewards: [1, 0, 1, 0] => value: 0.5
  std::string segment = "travel";
  processor.Process({segment, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);
  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  auto iter = arms.find(segment);
  targeting::EpsilonGreedyBanditArmInfo arm = iter->second;
  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 0.5;
  expected_arm.pulls = 4;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithFourRewards) {
  // Arrange
  processor::EpsilonGreedyBandit processor;

  // Act
  // rewards: [1, 1, 1, 1] => value: 1.0
  std::string segment = "travel";
  processor.Process({segment, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment, mojom::NotificationAdEventType::kClicked});

  // Assert
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);
  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  auto iter = arms.find(segment);
  targeting::EpsilonGreedyBanditArmInfo arm = iter->second;
  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 1.0;
  expected_arm.pulls = 4;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, ProcessSegmentNotInResource) {
  // Arrange
  processor::EpsilonGreedyBandit processor;

  // Act
  std::string segment = "foobar";
  processor.Process({segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);
  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  auto iter = arms.find(segment);
  EXPECT_TRUE(iter == arms.end());
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, ProcessChildSegment) {
  // Arrange
  processor::EpsilonGreedyBandit processor;

  // Act
  std::string segment = "travel-child";
  std::string parent_segment = "travel";
  processor.Process({segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  std::string json =
      AdsClientHelper::Get()->GetStringPref(prefs::kEpsilonGreedyBanditArms);
  targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(json);

  auto iter = arms.find(parent_segment);
  targeting::EpsilonGreedyBanditArmInfo arm = iter->second;
  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = parent_segment;
  expected_arm.value = 0.0;
  expected_arm.pulls = 1;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       InitializeArmsFromResourceWithEmptySegments) {
  // Arrange

  // Act
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::EpsilonGreedyBanditArms::FromJson(kArmsWithEmptySegmentJson);

  // Assert
  // Empty segments are skipped.
  EXPECT_EQ(1U, arms.size());
  EXPECT_EQ(1U, arms.count("travel"));
}

}  // namespace ads
