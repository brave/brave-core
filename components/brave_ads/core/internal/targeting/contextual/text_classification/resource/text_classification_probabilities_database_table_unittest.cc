/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_probabilities_database_table.h"

#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsTextClassificationProbabilitiesDatabaseTableTest
    : public test::TestBase {
 protected:
  TextClassificationProbabilities database_table_;
};

TEST_F(BraveAdsTextClassificationProbabilitiesDatabaseTableTest,
       SaveEmptyTextClassificationProbabilities) {
  // Act
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({}, base::Time::Now(), save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Assert
  base::test::TestFuture<bool, TextClassificationProbabilityList>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(text_classification_probabilities, ::testing::IsEmpty());
}

TEST_F(BraveAdsTextClassificationProbabilitiesDatabaseTableTest,
       SaveOrdersVisitsNewestFirst) {
  // Arrange
  const base::Time oldest_visit_at = base::Time::Now();
  const base::Time newest_visit_at = oldest_visit_at + base::Seconds(1);

  const TextClassificationProbabilityMap oldest_visit = {{"travel", 0.25}};
  const TextClassificationProbabilityMap newest_visit = {
      {"technology & computing", 0.5}};

  // Act
  base::test::TestFuture<bool> save_oldest_test_future;
  database_table_.Save(oldest_visit, oldest_visit_at,
                       save_oldest_test_future.GetCallback());
  ASSERT_TRUE(save_oldest_test_future.Take());

  base::test::TestFuture<bool> save_newest_test_future;
  database_table_.Save(newest_visit, newest_visit_at,
                       save_newest_test_future.GetCallback());
  ASSERT_TRUE(save_newest_test_future.Take());

  // Assert
  base::test::TestFuture<bool, TextClassificationProbabilityList>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(text_classification_probabilities,
              ::testing::ElementsAreArray({newest_visit, oldest_visit}));
}

TEST_F(BraveAdsTextClassificationProbabilitiesDatabaseTableTest,
       SavingTwoPagesAtTheSameTimeCreatesMultipleEntries) {
  // Arrange
  // Regression test: entries used to merge if their `created_at` collided.
  const base::Time created_at = base::Time::Now();

  const TextClassificationProbabilityMap first_entry = {{"travel", 0.25}};
  const TextClassificationProbabilityMap second_entry = {
      {"technology & computing", 0.5}};

  // Act
  base::test::TestFuture<bool> first_save_test_future;
  database_table_.Save(first_entry, created_at,
                       first_save_test_future.GetCallback());
  ASSERT_TRUE(first_save_test_future.Take());

  base::test::TestFuture<bool> second_save_test_future;
  database_table_.Save(second_entry, created_at,
                       second_save_test_future.GetCallback());
  ASSERT_TRUE(second_save_test_future.Take());

  // Assert
  base::test::TestFuture<bool, TextClassificationProbabilityList>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(
      text_classification_probabilities,
      ::testing::UnorderedElementsAreArray({first_entry, second_entry}));
}

TEST_F(BraveAdsTextClassificationProbabilitiesDatabaseTableTest,
       SaveWithMultipleSegments) {
  // Arrange
  const TextClassificationProbabilityMap probabilities = {
      {"travel", 0.25}, {"technology & computing", 0.5}, {"weather", 0.1}};

  // Act
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save(probabilities, base::Time::Now(),
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Assert
  base::test::TestFuture<bool, TextClassificationProbabilityList>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(text_classification_probabilities,
              ::testing::ElementsAreArray({probabilities}));
}

TEST_F(BraveAdsTextClassificationProbabilitiesDatabaseTableTest,
       PruneToMaximumEntriesDoesNothingWhenAtLimit) {
  // Arrange
  const base::Time now = base::Time::Now();

  for (int i = 0; i < 2; ++i) {
    base::test::TestFuture<bool> save_test_future;
    database_table_.Save({{"travel", 0.1 * i}}, now + base::Seconds(i),
                         save_test_future.GetCallback());
    ASSERT_TRUE(save_test_future.Take());
  }

  // Act
  base::test::TestFuture<bool> prune_test_future;
  database_table_.PruneToMaximumEntries(/*maximum_entries=*/2,
                                        prune_test_future.GetCallback());
  ASSERT_TRUE(prune_test_future.Take());

  // Assert
  base::test::TestFuture<bool, TextClassificationProbabilityList>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(text_classification_probabilities,
              ::testing::ElementsAreArray(
                  {TextClassificationProbabilityMap{{"travel", 0.1}},
                   TextClassificationProbabilityMap{{"travel", 0}}}));
}

TEST_F(BraveAdsTextClassificationProbabilitiesDatabaseTableTest,
       PruneToMaximumEntries) {
  // Arrange
  const base::Time now = base::Time::Now();

  for (int i = 0; i < 3; ++i) {
    base::test::TestFuture<bool> save_test_future;
    database_table_.Save({{"travel", 0.1 * i}}, now + base::Seconds(i),
                         save_test_future.GetCallback());
    ASSERT_TRUE(save_test_future.Take());
  }

  // Act
  base::test::TestFuture<bool> prune_test_future;
  database_table_.PruneToMaximumEntries(/*maximum_entries=*/2,
                                        prune_test_future.GetCallback());
  ASSERT_TRUE(prune_test_future.Take());

  // Assert
  base::test::TestFuture<bool, TextClassificationProbabilityList>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(2U, text_classification_probabilities.size());
  EXPECT_THAT(text_classification_probabilities,
              ::testing::ElementsAreArray(
                  {TextClassificationProbabilityMap{{"travel", 0.2}},
                   TextClassificationProbabilityMap{{"travel", 0.1}}}));
}

TEST_F(BraveAdsTextClassificationProbabilitiesDatabaseTableTest,
       DeleteAllTextClassificationProbabilities) {
  // Arrange
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({{"travel", 0.5}}, base::Time::Now(),
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  base::test::TestFuture<bool> delete_all_test_future;
  database_table_.DeleteAll(delete_all_test_future.GetCallback());
  ASSERT_TRUE(delete_all_test_future.Take());

  // Assert
  base::test::TestFuture<bool, TextClassificationProbabilityList>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const TextClassificationProbabilityList&>());
  const auto [success, text_classification_probabilities] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(text_classification_probabilities, ::testing::IsEmpty());
}

}  // namespace brave_ads::database::table
