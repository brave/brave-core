/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_text_classification_probabilities_json_parser.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::json::reader {

class BraveAdsLegacyClientMigrationTextClassificationProbabilitiesJsonParserTest
    : public ::testing::Test {};

TEST_F(
    BraveAdsLegacyClientMigrationTextClassificationProbabilitiesJsonParserTest,
    ParseTextClassificationProbabilities) {
  // Act
  const std::optional<TextClassificationProbabilityList>
      text_classification_probabilities = ParseTextClassificationProbabilities(
          R"JSON({
    "textClassificationProbabilitiesHistory": [
      {
        "textClassificationProbabilities": [
          { "segment": "technology & computing", "pageScore": "0.5" },
          { "segment": "travel", "pageScore": "0.25" }
        ]
      },
      {
        "textClassificationProbabilities": [
          { "segment": "travel", "pageScore": "0.75" }
        ]
      }
    ]
  })JSON");

  // Assert
  ASSERT_TRUE(text_classification_probabilities);
  EXPECT_THAT(*text_classification_probabilities,
              ::testing::ElementsAreArray(
                  {TextClassificationProbabilityMap{
                       {"technology & computing", 0.5}, {"travel", 0.25}},
                   TextClassificationProbabilityMap{{"travel", 0.75}}}));
}

TEST_F(
    BraveAdsLegacyClientMigrationTextClassificationProbabilitiesJsonParserTest,
    ParseEmptyTextClassificationProbabilities) {
  // Act
  const std::optional<TextClassificationProbabilityList>
      text_classification_probabilities =
          ParseTextClassificationProbabilities(R"JSON({})JSON");

  // Assert
  ASSERT_TRUE(text_classification_probabilities);
  EXPECT_THAT(*text_classification_probabilities, ::testing::IsEmpty());
}

TEST_F(
    BraveAdsLegacyClientMigrationTextClassificationProbabilitiesJsonParserTest,
    DoNotParseMalformedJson) {
  // Act & Assert
  EXPECT_FALSE(ParseTextClassificationProbabilities("{"));
}

}  // namespace brave_ads::json::reader
