/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"

#include <string>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/bandits/epsilon_greedy_bandit_model_unittest_util.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_segments.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {
namespace model {

class BatAdsEpsilonGreedyBanditModelTest : public UnitTestBase {
 protected:
  BatAdsEpsilonGreedyBanditModelTest() = default;

  ~BatAdsEpsilonGreedyBanditModelTest() override = default;
};

TEST_F(BatAdsEpsilonGreedyBanditModelTest,
       GetSegmentsIfProcessorNeverInitialized) {
  // Arrange
  SaveAllSegments();

  // Act
  EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, EligableSegmentsAreEmpty) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.5"}});

  processor::EpsilonGreedyBandit processor;

  // Act
  EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsIfNeverProcessed) {
  // Arrange
  SaveAllSegments();

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.25"}});

  processor::EpsilonGreedyBandit processor;

  // Act
  EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsForExploration) {
  // Arrange
  SaveAllSegments();

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "1.0"}});

  processor::EpsilonGreedyBandit processor;

  const std::string segment_1 = "travel";
  processor.Process({segment_1, mojom::AdNotificationEventType::kDismissed});
  const std::string segment_2 = "personal finance";
  processor.Process({segment_2, mojom::AdNotificationEventType::kClicked});

  // Act
  EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Exploration is non-deterministic, so can only verify the number of
  // segments returned
  EXPECT_EQ(3U, segments.size());
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsForExploitation) {
  // Arrange
  SaveAllSegments();

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.0"}});

  // Set all values to zero by choosing a zero-reward action due to
  // optimistic initial values for arms
  processor::EpsilonGreedyBandit processor;
  for (const auto& segment : kSegments) {
    processor.Process({segment, mojom::AdNotificationEventType::kDismissed});
  }

  std::string segment_1 = "science";
  processor.Process({segment_1, mojom::AdNotificationEventType::kClicked});
  processor.Process({segment_1, mojom::AdNotificationEventType::kClicked});
  processor.Process({segment_1, mojom::AdNotificationEventType::kClicked});

  std::string segment_2 = "travel";
  processor.Process({segment_2, mojom::AdNotificationEventType::kDismissed});
  processor.Process({segment_2, mojom::AdNotificationEventType::kClicked});
  processor.Process({segment_2, mojom::AdNotificationEventType::kClicked});

  std::string segment_3 = "technology & computing";
  processor.Process({segment_3, mojom::AdNotificationEventType::kDismissed});
  processor.Process({segment_3, mojom::AdNotificationEventType::kDismissed});
  processor.Process({segment_3, mojom::AdNotificationEventType::kClicked});

  // Act
  EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"science", "travel",
                                         "technology & computing"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsEpsilonGreedyBanditModelTest, GetSegmentsForEligibleSegments) {
  // Arrange
  const std::vector<std::string> eligible_segments = {
      "science", "technology & computing", "invalid_segment"};
  SaveSegments(eligible_segments);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kEpsilonGreedyBandit, {{"epsilon_value", "0.0"}});

  // Set all values to zero by choosing a zero-reward action due to
  // optimistic initial values for arms
  processor::EpsilonGreedyBandit processor;
  for (const auto& segment : kSegments) {
    processor.Process({segment, mojom::AdNotificationEventType::kDismissed});
  }

  std::string segment_1 = "science";
  processor.Process({segment_1, mojom::AdNotificationEventType::kClicked});
  processor.Process({segment_1, mojom::AdNotificationEventType::kClicked});
  processor.Process({segment_1, mojom::AdNotificationEventType::kClicked});

  std::string segment_2 = "travel";
  processor.Process({segment_2, mojom::AdNotificationEventType::kDismissed});
  processor.Process({segment_2, mojom::AdNotificationEventType::kClicked});
  processor.Process({segment_2, mojom::AdNotificationEventType::kClicked});

  std::string segment_3 = "technology & computing";
  processor.Process({segment_3, mojom::AdNotificationEventType::kDismissed});
  processor.Process({segment_3, mojom::AdNotificationEventType::kDismissed});
  processor.Process({segment_3, mojom::AdNotificationEventType::kClicked});

  // Act
  EpsilonGreedyBandit model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"science", "technology & computing"};

  EXPECT_EQ(expected_segments, segments);
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
