/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_database_table.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_probabilities_database_table.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kClientWithPurchaseIntentAndTextClassificationJsonFilename[] =
    "client_with_purchase_intent_and_text_classification.json";
constexpr char kClientWithNoDataJsonFilename[] = "client_with_no_data.json";

size_t GetPurchaseIntentSignalHistorySegmentCount() {
  base::test::TestFuture<bool, PurchaseIntentSignalHistoryMap> test_future;
  database::table::PurchaseIntentSignalHistory().GetAll(
      test_future.GetCallback<bool, const PurchaseIntentSignalHistoryMap&>());
  const auto [success, purchase_intent_signal_history] = test_future.Take();
  EXPECT_TRUE(success);
  return purchase_intent_signal_history.size();
}

size_t GetTextClassificationProbabilitiesVisitCount() {
  base::test::TestFuture<bool, TextClassificationProbabilityList> test_future;
  database::table::TextClassificationProbabilities().GetAll(
      test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] = test_future.Take();
  EXPECT_TRUE(success);
  return text_classification_probabilities.size();
}

}  // namespace

class BraveAdsLegacyClientMigrationTest : public test::TestBase {};

TEST_F(BraveAdsLegacyClientMigrationTest,
       MigrateWithPurchaseIntentAndTextClassification) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kClientWithPurchaseIntentAndTextClassificationJsonFilename,
      kClientJsonFilename));

  EXPECT_CALL(ads_client_mock_, Remove(kClientJsonFilename, ::testing::_));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateClientState(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(2U, GetPurchaseIntentSignalHistorySegmentCount());
  EXPECT_EQ(2U, GetTextClassificationProbabilitiesVisitCount());
}

TEST_F(BraveAdsLegacyClientMigrationTest, MigrateWithNoData) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      kClientWithNoDataJsonFilename, kClientJsonFilename));

  EXPECT_CALL(ads_client_mock_, Remove(kClientJsonFilename, ::testing::_));

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateClientState(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(0U, GetPurchaseIntentSignalHistorySegmentCount());
  EXPECT_EQ(0U, GetTextClassificationProbabilitiesVisitCount());
}

TEST_F(BraveAdsLegacyClientMigrationTest, MigrateWhenClientStateIsMalformed) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      test::kMalformedJsonFilename, kClientJsonFilename));

  EXPECT_CALL(ads_client_mock_, Remove(kClientJsonFilename, ::testing::_));

  // Act & Assert
  base::test::TestFuture<bool> test_future;
  MigrateClientState(test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsLegacyClientMigrationTest, MigrateWhenClientStateDoesNotExist) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, Remove(kClientJsonFilename, ::testing::_))
      .Times(0);

  // Act
  base::test::TestFuture<bool> test_future;
  MigrateClientState(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_EQ(0U, GetPurchaseIntentSignalHistorySegmentCount());
  EXPECT_EQ(0U, GetTextClassificationProbabilitiesVisitCount());
}

}  // namespace brave_ads
