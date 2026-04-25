/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/keyphrase/purchase_intent_keyphrase_parser.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPurchaseIntentKeyphraseParserTest, ParseKeyphrase) {
  // Act
  const KeywordList keywords = ParseKeyphrase(R"("Audi RS6" Automobile)");

  // Assert
  const KeywordList expected_keywords = {"audi rs6", "automobile"};
  EXPECT_EQ(expected_keywords, keywords);
}

TEST(BraveAdsPurchaseIntentKeyphraseParserTest,
     ParsePurchaseIntentKeyphraseStrippingWhitespace) {
  // Act
  const KeywordList keywords =
      ParseKeyphrase(R"("  Mercedes AMG E63S "  Automobile  )");

  // Assert
  const KeywordList expected_keywords = {"mercedes amg e63s", "automobile"};
  EXPECT_EQ(expected_keywords, keywords);
}

TEST(BraveAdsPurchaseIntentKeyphraseParserTest, ParseEmptyKeyphrase) {
  // Act
  const KeywordList keywords = ParseKeyphrase("");

  // Assert
  EXPECT_THAT(keywords, ::testing::IsEmpty());
}

}  // namespace brave_ads
