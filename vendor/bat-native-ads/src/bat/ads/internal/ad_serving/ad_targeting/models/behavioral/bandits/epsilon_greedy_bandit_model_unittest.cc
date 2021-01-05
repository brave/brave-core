/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"

#include <string>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_targeting/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

class BatAdsEpsilonGreedyBanditModelTest : public UnitTestBase {
 protected:
  BatAdsEpsilonGreedyBanditModelTest() = default;

  ~BatAdsEpsilonGreedyBanditModelTest() override = default;
};

TEST_F(BatAdsEpsilonGreedyBanditModelTest,
    GetSegmentsIfProcessorNeverInitialized) {
  // Arrange

  // Act
  model::EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest,
    GetSegmentsIfNeverProcessed) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.25"}});

  processor::EpsilonGreedyBandit processor;

  // Act
  model::EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest,
    GetSegmentsForExploration) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "1.0"}});

  processor::EpsilonGreedyBandit processor;

  const std::string segment_1 = "travel";
  processor.Process({segment_1, AdNotificationEventType::kDismissed});
  const std::string segment_2 = "personal finance";
  processor.Process({segment_2, AdNotificationEventType::kClicked});

  // Act
  model::EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Exploration is non-deterministic, so can only verify the number of
  // segments returned
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest,
    GetSegmentsForExploitation) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.0"}});

  // Set all values to zero by choosing a zero-reward action due to
  // optimistic initial values for arms
  processor::EpsilonGreedyBandit processor;
  for (const auto& segment : resource::kSegments) {
    processor.Process({segment, AdNotificationEventType::kDismissed});
  }

  std::string segment_1 = "science";
  processor.Process({segment_1, AdNotificationEventType::kClicked});
  processor.Process({segment_1, AdNotificationEventType::kClicked});
  processor.Process({segment_1, AdNotificationEventType::kClicked});

  std::string segment_2 = "travel";
  processor.Process({segment_2, AdNotificationEventType::kDismissed});
  processor.Process({segment_2, AdNotificationEventType::kClicked});
  processor.Process({segment_2, AdNotificationEventType::kClicked});

  std::string segment_3 = "technology & computing";
  processor.Process({segment_3, AdNotificationEventType::kDismissed});
  processor.Process({segment_3, AdNotificationEventType::kDismissed});
  processor.Process({segment_3, AdNotificationEventType::kClicked});

  // Act
  model::EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {
    "science",
    "travel",
    "technology & computing"
  };

  EXPECT_EQ(expected_segments, segments);
}

}  // namespace ad_targeting
}  // namespace ads
