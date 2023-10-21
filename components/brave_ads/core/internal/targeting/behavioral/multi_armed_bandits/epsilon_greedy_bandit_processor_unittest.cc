/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"

#include <string>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arm_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEpsilonGreedyBanditProcessorTest : public UnitTestBase {};

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest, InitializeArmsFromResource) {
  // Arrange
  {
    EpsilonGreedyBanditArmMap arms;

    EpsilonGreedyBanditArmInfo arm_1;
    arm_1.segment = "foo";
    arm_1.pulls = 0;
    arm_1.value = 1.0;
    arms["foo"] = arm_1;

    EpsilonGreedyBanditArmInfo arm_2;
    arm_2.segment = "bar";
    arm_2.pulls = 0;
    arm_2.value = 1.0;
    arms["bar"] = arm_2;

    SetEpsilonGreedyBanditArms(arms);
  }

  const EpsilonGreedyBanditProcessor processor;

  // Act
  NotifyDidInitializeAds();

  // Assert
  const EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();
  EXPECT_EQ(27U, arms.size());
  EXPECT_EQ(0U, arms.count("foo"));
  EXPECT_EQ(0U, arms.count("bar"));
}

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest, NeverProcessed) {
  // Arrange
  const std::string segment = "travel";  // rewards: [] => value: 1.0

  const EpsilonGreedyBanditProcessor processor;

  // Act
  NotifyDidInitializeAds();

  // Assert
  const EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const EpsilonGreedyBanditArmInfo arm = iter->second;

  EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 1.0;
  expected_arm.pulls = 0;
  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithOneReward) {
  // Arrange
  const std::string segment = "travel";  // rewards: [0, 0, 0, 0] => value: 0.0

  const EpsilonGreedyBanditProcessor processor;

  NotifyDidInitializeAds();

  // Act
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kDismissed});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kDismissed});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kTimedOut});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kDismissed});

  // Assert
  const EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const EpsilonGreedyBanditArmInfo arm = iter->second;

  EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 0.0;
  expected_arm.pulls = 4;
  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithTwoRewards) {
  // Arrange
  const std::string segment = "travel";  // rewards: [1, 0, 1, 0] => value: 0.5

  const EpsilonGreedyBanditProcessor processor;

  NotifyDidInitializeAds();

  // Act
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kClicked});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kDismissed});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kClicked});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  const EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const EpsilonGreedyBanditArmInfo arm = iter->second;

  EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 0.5;
  expected_arm.pulls = 4;
  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithFourRewards) {
  // Arrange
  const std::string segment = "travel";  // rewards: [1, 1, 1, 1] => value: 1.0

  const EpsilonGreedyBanditProcessor processor;

  NotifyDidInitializeAds();

  // Act
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kClicked});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kClicked});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kClicked});
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kClicked});

  // Assert
  const EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const EpsilonGreedyBanditArmInfo arm = iter->second;

  EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 1.0;
  expected_arm.pulls = 4;
  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest, ProcessSegmentNotInResource) {
  // Arrange
  const std::string segment = "foobar";

  const EpsilonGreedyBanditProcessor processor;

  NotifyDidInitializeAds();

  // Act
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  EXPECT_FALSE(base::Contains(GetEpsilonGreedyBanditArms(), segment));
}

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest, ProcessChildSegment) {
  // Arrange
  const std::string segment = "travel-child";
  const std::string parent_segment = "travel";

  const EpsilonGreedyBanditProcessor processor;

  NotifyDidInitializeAds();

  // Act
  processor.Process(EpsilonGreedyBanditFeedbackInfo{
      segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  const EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(parent_segment);
  ASSERT_TRUE(iter != arms.cend());
  const EpsilonGreedyBanditArmInfo arm = iter->second;

  EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = parent_segment;
  expected_arm.value = 0.0;
  expected_arm.pulls = 1;
  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BraveAdsEpsilonGreedyBanditProcessorTest,
       InitializeArmsFromResourceWithEmptySegments) {
  // Arrange
  EpsilonGreedyBanditArmMap arms;

  EpsilonGreedyBanditArmInfo arm_1;
  arm_1.segment = "travel";
  arm_1.pulls = 0;
  arm_1.value = 1.0;
  arms["travel"] = arm_1;

  EpsilonGreedyBanditArmInfo arm_2;
  arm_2.pulls = 0;
  arm_2.value = 1.0;
  arms[""] = arm_2;

  // Act
  SetEpsilonGreedyBanditArms(arms);

  // Assert
  const EpsilonGreedyBanditArmMap expected_arms = GetEpsilonGreedyBanditArms();
  EXPECT_EQ(1U, expected_arms.size());
  EXPECT_EQ(1U, expected_arms.count("travel"));
}

}  // namespace brave_ads
