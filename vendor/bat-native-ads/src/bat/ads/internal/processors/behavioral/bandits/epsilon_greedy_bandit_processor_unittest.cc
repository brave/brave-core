/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"

#include <string>

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arm_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEpsilonGreedyBanditProcessorTest : public UnitTestBase {};

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, InitializeArmsFromResource) {
  // Arrange
  {
    targeting::EpsilonGreedyBanditArmMap arms;

    targeting::EpsilonGreedyBanditArmInfo arm_1;
    arm_1.segment = "foo";
    arm_1.pulls = 0;
    arm_1.value = 1.0;
    arms["foo"] = arm_1;

    targeting::EpsilonGreedyBanditArmInfo arm_2;
    arm_2.segment = "bar";
    arm_2.pulls = 0;
    arm_2.value = 1.0;
    arms["bar"] = arm_2;

    targeting::SetEpsilonGreedyBanditArms(arms);
  }

  // Act
  const processor::EpsilonGreedyBandit processor;

  // Assert
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();

  EXPECT_EQ(30U, arms.size());
  EXPECT_EQ(0U, arms.count("foo"));
  EXPECT_EQ(0U, arms.count("bar"));
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, NeverProcessed) {
  // Arrange
  const std::string segment = "travel";  // rewards: [] => value: 1.0

  // Act
  const processor::EpsilonGreedyBandit processor;

  // Assert
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const targeting::EpsilonGreedyBanditArmInfo arm = iter->second;

  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 1.0;
  expected_arm.pulls = 0;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithOneReward) {
  // Arrange
  const std::string segment = "travel";  // rewards: [0, 0, 0, 0] => value: 0.0

  // Act
  const processor::EpsilonGreedyBandit processor;
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kTimedOut});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kDismissed});

  // Assert
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const targeting::EpsilonGreedyBanditArmInfo arm = iter->second;

  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 0.0;
  expected_arm.pulls = 4;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithTwoRewards) {
  // Arrange
  const std::string segment = "travel";  // rewards: [1, 0, 1, 0] => value: 0.5

  // Act
  const processor::EpsilonGreedyBandit processor;
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const targeting::EpsilonGreedyBanditArmInfo arm = iter->second;

  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 0.5;
  expected_arm.pulls = 4;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       ProcessSegmentFourTimesWithFourRewards) {
  // Arrange
  const std::string segment = "travel";  // rewards: [1, 1, 1, 1] => value: 1.0

  // Act
  const processor::EpsilonGreedyBandit processor;
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kClicked});

  // Assert
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  ASSERT_TRUE(iter != arms.cend());
  const targeting::EpsilonGreedyBanditArmInfo arm = iter->second;

  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = segment;
  expected_arm.value = 1.0;
  expected_arm.pulls = 4;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, ProcessSegmentNotInResource) {
  // Arrange
  const std::string segment = "foobar";

  // Act
  const processor::EpsilonGreedyBandit processor;
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(segment);
  EXPECT_TRUE(iter == arms.cend());
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest, ProcessChildSegment) {
  // Arrange
  const std::string segment = "travel-child";
  const std::string parent_segment = "travel";

  // Act
  const processor::EpsilonGreedyBandit processor;
  processor::EpsilonGreedyBandit::Process(
      {segment, mojom::NotificationAdEventType::kTimedOut});

  // Assert
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();
  const auto iter = arms.find(parent_segment);
  ASSERT_TRUE(iter != arms.cend());
  const targeting::EpsilonGreedyBanditArmInfo arm = iter->second;

  targeting::EpsilonGreedyBanditArmInfo expected_arm;
  expected_arm.segment = parent_segment;
  expected_arm.value = 0.0;
  expected_arm.pulls = 1;

  EXPECT_EQ(expected_arm, arm);
}

TEST_F(BatAdsEpsilonGreedyBanditProcessorTest,
       InitializeArmsFromResourceWithEmptySegments) {
  // Arrange
  {
    targeting::EpsilonGreedyBanditArmMap arms;

    targeting::EpsilonGreedyBanditArmInfo arm_1;
    arm_1.segment = "travel";
    arm_1.pulls = 0;
    arm_1.value = 1.0;
    arms["travel"] = arm_1;

    targeting::EpsilonGreedyBanditArmInfo arm_2;
    arm_2.segment = {};
    arm_2.pulls = 0;
    arm_2.value = 1.0;
    arms[""] = arm_2;

    targeting::SetEpsilonGreedyBanditArms(arms);
  }

  // Act
  const targeting::EpsilonGreedyBanditArmMap arms =
      targeting::GetEpsilonGreedyBanditArms();

  // Assert
  EXPECT_EQ(1U, arms.size());
  EXPECT_EQ(1U, arms.count("travel"));
}

}  // namespace ads
