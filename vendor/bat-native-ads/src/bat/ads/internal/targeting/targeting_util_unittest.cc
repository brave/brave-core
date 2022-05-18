/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/targeting/targeting_util.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/targeting/targeting_user_model_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace targeting {

class BatAdsTargetingUtilTest : public UnitTestBase {
 protected:
  BatAdsTargetingUtilTest() = default;

  ~BatAdsTargetingUtilTest() override = default;
};

TEST_F(BatAdsTargetingUtilTest, GetTopChildSegments) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest_segments = {"interest-1", "interest-2"};
  user_model.latent_interest_segments = {"latent_interest-1",
                                         "latent_interest-2"};
  user_model.purchase_intent_segments = {"purchase_intent-1",
                                         "purchase_intent-2"};

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ false);

  // Assert
  const SegmentList expected_segments = {
      "interest-1",        "interest-2",        "latent_interest-1",
      "latent_interest-2", "purchase_intent-1", "purchase_intent-2"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTargetingUtilTest, GetTopChildSegmentsForEmptyUserModel) {
  // Arrange
  const UserModelInfo user_model;

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ false);

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsTargetingUtilTest, GetTopParentSegments) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest_segments = {"interest_1", "interest_2"};
  user_model.latent_interest_segments = {"latent_interest_1",
                                         "latent_interest_2"};
  user_model.purchase_intent_segments = {"purchase_intent_1",
                                         "purchase_intent_2"};

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ true);

  // Assert
  const SegmentList expected_segments = {
      "interest_1",        "interest_2",        "latent_interest_1",
      "latent_interest_2", "purchase_intent_1", "purchase_intent_2"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTargetingUtilTest, GetTopParentSegmentsForEmptyUserModel) {
  // Arrange
  const UserModelInfo user_model;

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ true);

  // Assert
  EXPECT_TRUE(segments.empty());
}

}  // namespace targeting
}  // namespace ads
