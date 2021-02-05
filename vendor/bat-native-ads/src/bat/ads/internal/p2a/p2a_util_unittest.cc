/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/p2a/p2a_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace p2a {

TEST(BatAdsP2AUtilTest, CreateAdOpportunityQuestionList) {
  // Arrange
  std::vector<std::string> segments = {"technology & computing",
                                       "personal finance-crypto", "travel"};

  // Act
  const std::vector<std::string> questions =
      CreateAdOpportunityQuestionList(segments);

  // Assert
  const std::vector<std::string> expected_questions = {
      "Brave.P2A.AdOpportunitiesPerSegment.technologycomputing",
      "Brave.P2A.AdOpportunitiesPerSegment.personalfinance",
      "Brave.P2A.AdOpportunitiesPerSegment.travel",
      "Brave.P2A.TotalAdOpportunities"};

  EXPECT_EQ(expected_questions, questions);
}

TEST(BatAdsP2AUtilTest, CreateAdOpportunityQuestionListForEmptySegmentList) {
  // Arrange
  std::vector<std::string> segments = {};

  // Act
  const std::vector<std::string> questions =
      CreateAdOpportunityQuestionList(segments);

  // Assert
  const std::vector<std::string> expected_questions = {
      "Brave.P2A.TotalAdOpportunities"};

  EXPECT_EQ(expected_questions, questions);
}

TEST(BatAdsP2AUtilTest, CreateAdImpressionQuestionList) {
  // Arrange
  std::string segment = "technology & computing-software";

  // Act
  const std::vector<std::string> questions =
      CreateAdImpressionQuestionList(segment);

  // Assert
  const std::vector<std::string> expected_questions = {
      "Brave.P2A.AdImpressionsPerSegment.technologycomputing",
      "Brave.P2A.TotalAdImpressions"};

  EXPECT_EQ(expected_questions, questions);
}

TEST(BatAdsP2AUtilTest, CreateAdImpressionQuestionListForEmptySegment) {
  // Arrange
  std::string segment;

  // Act
  const std::vector<std::string> questions =
      CreateAdImpressionQuestionList(segment);

  // Assert
  const std::vector<std::string> expected_questions = {
      "Brave.P2A.TotalAdImpressions"};

  EXPECT_EQ(expected_questions, questions);
}

}  // namespace p2a
}  // namespace ads
