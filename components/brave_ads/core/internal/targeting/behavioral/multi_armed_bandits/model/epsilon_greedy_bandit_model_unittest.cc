/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/model/epsilon_greedy_bandit_model.h"

#include <iterator>
#include <string>

#include "base/strings/string_piece.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_segments.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

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

class BraveAdsEpsilonGreedyBanditModelTest : public UnitTestBase {};

TEST_F(BraveAdsEpsilonGreedyBanditModelTest,
       GetSegmentsIfProcessorNeverInitialized) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  const EpsilonGreedyBanditModel model;
  NotifyDidInitializeAds();

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BraveAdsEpsilonGreedyBanditModelTest, EligableSegmentsAreEmpty) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEpsilonGreedyBanditFeatures, {{"epsilon_value", "0.5"}});

  const EpsilonGreedyBanditProcessor processor;
  const EpsilonGreedyBanditModel model;
  NotifyDidInitializeAds();

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BraveAdsEpsilonGreedyBanditModelTest, GetSegmentsIfNeverProcessed) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEpsilonGreedyBanditFeatures, {{"epsilon_value", "0.25"}});

  const EpsilonGreedyBanditProcessor processor;
  const EpsilonGreedyBanditModel model;
  NotifyDidInitializeAds();

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BraveAdsEpsilonGreedyBanditModelTest, GetSegmentsForExploration) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEpsilonGreedyBanditFeatures, {{"epsilon_value", "1.0"}});

  const EpsilonGreedyBanditProcessor processor;

  processor.Process(
      {/*segment*/ "travel", mojom::NotificationAdEventType::kDismissed});
  processor.Process({/*segment*/ "personal finance",
                     mojom::NotificationAdEventType::kClicked});

  const EpsilonGreedyBanditModel model;
  NotifyDidInitializeAds();

  // Act
  const SegmentList segments = model.GetSegments();

  // Exploration is non-deterministic, so can only verify the number of
  // segments returned
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BraveAdsEpsilonGreedyBanditModelTest, GetSegmentsForExploitation) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEpsilonGreedyBanditFeatures, {{"epsilon_value", "0.0"}});

  // Set all values to zero by choosing a zero-reward action due to
  // optimistic initial values for arms
  const EpsilonGreedyBanditProcessor processor;
  NotifyDidInitializeAds();

  for (const base::StringPiece segment : GetSegments()) {
    processor.Process({static_cast<std::string>(segment),
                       mojom::NotificationAdEventType::kDismissed});
  }

  const std::string segment_1 = "science";
  processor.Process({segment_1, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment_1, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment_1, mojom::NotificationAdEventType::kClicked});

  const std::string segment_2 = "travel";
  processor.Process({segment_2, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment_2, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment_2, mojom::NotificationAdEventType::kClicked});

  const std::string segment_3 = "technology & computing";
  processor.Process({segment_3, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment_3, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment_3, mojom::NotificationAdEventType::kClicked});

  const EpsilonGreedyBanditModel model;
  NotifyDidInitializeAds();

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"science", "travel",
                                         "technology & computing"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BraveAdsEpsilonGreedyBanditModelTest, GetSegmentsForEligibleSegments) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(
      {"science", "technology & computing", "invalid_segment"});

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEpsilonGreedyBanditFeatures, {{"epsilon_value", "0.0"}});

  // Set all values to zero by choosing a zero-reward action due to
  // optimistic initial values for arms
  const EpsilonGreedyBanditProcessor processor;
  for (const base::StringPiece segment : GetSegments()) {
    processor.Process({static_cast<std::string>(segment),
                       mojom::NotificationAdEventType::kDismissed});
  }

  const std::string segment_1 = "science";
  processor.Process({segment_1, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment_1, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment_1, mojom::NotificationAdEventType::kClicked});

  const std::string segment_2 = "travel";
  processor.Process({segment_2, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment_2, mojom::NotificationAdEventType::kClicked});
  processor.Process({segment_2, mojom::NotificationAdEventType::kClicked});

  const std::string segment_3 = "technology & computing";
  processor.Process({segment_3, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment_3, mojom::NotificationAdEventType::kDismissed});
  processor.Process({segment_3, mojom::NotificationAdEventType::kClicked});

  const EpsilonGreedyBanditModel model;
  NotifyDidInitializeAds();

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"science", "technology & computing"};
  EXPECT_EQ(expected_segments, segments);
}

}  // namespace brave_ads
