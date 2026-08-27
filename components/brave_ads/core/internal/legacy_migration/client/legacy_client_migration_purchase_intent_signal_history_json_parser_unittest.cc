/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_purchase_intent_signal_history_json_parser.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::json::reader {

class BraveAdsLegacyClientMigrationPurchaseIntentSignalHistoryJsonParserTest
    : public ::testing::Test {};

TEST_F(BraveAdsLegacyClientMigrationPurchaseIntentSignalHistoryJsonParserTest,
       ParsePurchaseIntentSignalHistory) {
  // Act
  const std::optional<PurchaseIntentSignalHistoryMap>
      purchase_intent_signal_history = ParsePurchaseIntentSignalHistory(
          R"JSON({
    "purchaseIntentSignalHistory": {
      "travel": [
        { "created_at": "13303073580000000", "weight": 1 },
        { "created_at": "13303073680000000", "weight": 2 }
      ]
    }
  })JSON");

  // Assert
  ASSERT_TRUE(purchase_intent_signal_history);
  ASSERT_EQ(1U, purchase_intent_signal_history->size());
  EXPECT_EQ(2U, purchase_intent_signal_history->at("travel").size());
  EXPECT_EQ(1, purchase_intent_signal_history->at("travel")[0].weight);
  EXPECT_EQ(2, purchase_intent_signal_history->at("travel")[1].weight);
}

TEST_F(BraveAdsLegacyClientMigrationPurchaseIntentSignalHistoryJsonParserTest,
       ParseEmptyPurchaseIntentSignalHistory) {
  // Act
  const std::optional<PurchaseIntentSignalHistoryMap>
      purchase_intent_signal_history =
          ParsePurchaseIntentSignalHistory(R"JSON({})JSON");

  // Assert
  ASSERT_TRUE(purchase_intent_signal_history);
  EXPECT_THAT(*purchase_intent_signal_history, ::testing::IsEmpty());
}

TEST_F(BraveAdsLegacyClientMigrationPurchaseIntentSignalHistoryJsonParserTest,
       DoNotParseMalformedJson) {
  // Act & Assert
  EXPECT_FALSE(ParsePurchaseIntentSignalHistory("{"));
}

}  // namespace brave_ads::json::reader
