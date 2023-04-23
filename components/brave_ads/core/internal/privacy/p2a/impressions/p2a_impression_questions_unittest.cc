/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/p2a/impressions/p2a_impression_questions.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::p2a {

TEST(BraveAdsP2AImpressionQuestionsTest, CreateAdImpressionQuestions) {
  // Arrange

  // Act
  const std::vector<std::string> questions = CreateAdImpressionQuestions(
      /*segment*/ "technology & computing-software");

  // Assert
  const std::vector<std::string> expected_questions = {
      "Brave.P2A.AdImpressionsPerSegment.technologycomputing",
      "Brave.P2A.TotalAdImpressions"};

  EXPECT_EQ(expected_questions, questions);
}

}  // namespace brave_ads::privacy::p2a
