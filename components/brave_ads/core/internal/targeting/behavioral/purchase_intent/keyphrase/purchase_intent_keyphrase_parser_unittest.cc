/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/keyphrase/purchase_intent_keyphrase_parser.h"

#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPurchaseIntentKeyphraseParserTest, ParseKeyphrase) {
  // Act & Assert
  const KeywordList expected_keywords = {"audi rs6", "automobile"};

  EXPECT_THAT(
      expected_keywords,
      ::testing::ElementsAreArray(ParseKeyphrase(R"("Audi RS6" Automobile)")));
}

TEST(BraveAdsPurchaseIntentKeyphraseParserTest,
     ParsePurchaseIntentKeyphraseStrippingWhitespace) {
  // Act & Assert
  const KeywordList expected_keywords = {"mercedes amg e63s", "automobile"};

  EXPECT_THAT(expected_keywords,
              ::testing::ElementsAreArray(
                  ParseKeyphrase(R"("  Mercedes AMG E63S "  Automobile  )")));
}

TEST(BraveAdsPurchaseIntentKeyphraseParserTest, ParseEmptyKeyphrase) {
  // Act & Asysert
  EXPECT_THAT(ParseKeyphrase(""), ::testing::IsEmpty());
}

}  // namespace brave_ads
