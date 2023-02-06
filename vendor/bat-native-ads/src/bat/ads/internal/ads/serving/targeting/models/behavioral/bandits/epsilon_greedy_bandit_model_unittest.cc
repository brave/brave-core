/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"

#include <string>

#include "base/strings/string_piece.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_segments.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::targeting::model {

namespace {

SegmentList GetSegmentList() {
  SegmentList segments;
  base::ranges::transform(GetSegments(), std::back_inserter(segments),
                          [](const base::StringPiece& segment) {
                            return static_cast<std::string>(segment);
                          });
  return segments;
}

}  // namespace

class BatAdsEpsilonGreedyBanditModelTest : public UnitTestBase {};

TEST_F(BatAdsEpsilonGreedyBanditModelTest,
       GetSegmentsIfProcessorNeverInitialized) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  // Act
  const EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, EligableSegmentsAreEmpty) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.5"}});

  const processor::EpsilonGreedyBandit processor;

  // Act
  const EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsIfNeverProcessed) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.25"}});

  const processor::EpsilonGreedyBandit processor;

  // Act
  const EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsForExploration) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "1.0"}});

  const processor::EpsilonGreedyBandit processor;

  const std::string segment_1 = "travel";
  processor::EpsilonGreedyBandit::Process(
      {segment_1, mojom::NotificationAdEventType::kDismissed});
  const std::string segment_2 = "personal finance";
  processor::EpsilonGreedyBandit::Process(
      {segment_2, mojom::NotificationAdEventType::kClicked});

  // Act
  const EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Exploration is non-deterministic, so can only verify the number of
  // segments returned
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsForExploitation) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.0"}});

  // Set all values to zero by choosing a zero-reward action due to
  // optimistic initial values for arms
  const processor::EpsilonGreedyBandit processor;
  for (const base::StringPiece segment : GetSegments()) {
    processor::EpsilonGreedyBandit::Process(
        {static_cast<std::string>(segment),
         mojom::NotificationAdEventType::kDismissed});
  }

  const std::string segment_1 = "science";
  processor::EpsilonGreedyBandit::Process(
      {segment_1, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment_1, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment_1, mojom::NotificationAdEventType::kClicked});

  const std::string segment_2 = "travel";
  processor::EpsilonGreedyBandit::Process(
      {segment_2, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment_2, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment_2, mojom::NotificationAdEventType::kClicked});

  const std::string segment_3 = "technology & computing";
  processor::EpsilonGreedyBandit::Process(
      {segment_3, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment_3, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment_3, mojom::NotificationAdEventType::kClicked});

  // Act
  const EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"science", "travel",
                                         "technology & computing"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsForEligibleSegments) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(
      {"science", "technology & computing", "invalid_segment"});

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.0"}});

  // Set all values to zero by choosing a zero-reward action due to
  // optimistic initial values for arms
  const processor::EpsilonGreedyBandit processor;
  for (const base::StringPiece segment : GetSegments()) {
    processor::EpsilonGreedyBandit::Process(
        {static_cast<std::string>(segment),
         mojom::NotificationAdEventType::kDismissed});
  }

  const std::string segment_1 = "science";
  processor::EpsilonGreedyBandit::Process(
      {segment_1, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment_1, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment_1, mojom::NotificationAdEventType::kClicked});

  const std::string segment_2 = "travel";
  processor::EpsilonGreedyBandit::Process(
      {segment_2, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment_2, mojom::NotificationAdEventType::kClicked});
  processor::EpsilonGreedyBandit::Process(
      {segment_2, mojom::NotificationAdEventType::kClicked});

  const std::string segment_3 = "technology & computing";
  processor::EpsilonGreedyBandit::Process(
      {segment_3, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment_3, mojom::NotificationAdEventType::kDismissed});
  processor::EpsilonGreedyBandit::Process(
      {segment_3, mojom::NotificationAdEventType::kClicked});

  // Act
  const EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"science", "technology & computing"};

  EXPECT_EQ(expected_segments, segments);
}

}  // namespace ads::targeting::model
