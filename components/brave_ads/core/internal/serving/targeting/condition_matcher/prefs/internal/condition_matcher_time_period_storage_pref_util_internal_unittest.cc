/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_time_period_storage_pref_util_internal.h"

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConditionMatcherTimePeriodStorageUtilTest
    : public test::TestBase {};

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       ResolveCutoffForDefaultDuration) {
  // Act & Assert
  EXPECT_EQ(base::Time(),
            MaybeResolveTimePeriodStorageCutoff("time_period_storage"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       ResolveCutoffForEmptyDuration) {
  // Act & Assert
  EXPECT_EQ(base::Time(),
            MaybeResolveTimePeriodStorageCutoff("time_period_storage="));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       ResolveCutoffForAllDuration) {
  // Act & Assert
  EXPECT_EQ(base::Time(),
            MaybeResolveTimePeriodStorageCutoff("time_period_storage=all"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       ResolveCutoffForDaysDuration) {
  // Act & Assert
  EXPECT_EQ(test::Now() - base::Days(7),
            MaybeResolveTimePeriodStorageCutoff("time_period_storage=7d"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       ResolveCutoffForHoursDuration) {
  // Act & Assert
  EXPECT_EQ(test::Now() - base::Hours(1),
            MaybeResolveTimePeriodStorageCutoff("time_period_storage=1h"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       ResolveCutoffForMinutesDuration) {
  // Act & Assert
  EXPECT_EQ(test::Now() - base::Minutes(30),
            MaybeResolveTimePeriodStorageCutoff("time_period_storage=30m"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       ResolveCutoffForSecondsDuration) {
  // Act & Assert
  EXPECT_EQ(test::Now() - base::Seconds(60),
            MaybeResolveTimePeriodStorageCutoff("time_period_storage=60s"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       DoNotResolveCutoffForInvalidDuration) {
  // Act & Assert
  EXPECT_FALSE(
      MaybeResolveTimePeriodStorageCutoff("time_period_storage=invalid"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       DoNotResolveCutoffForNonTimePeriodStoragePattern) {
  // Act & Assert
  EXPECT_FALSE(MaybeResolveTimePeriodStorageCutoff("not_time_period_storage"));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesForEmptyList) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, SumTimePeriodStorageListValues(base::ListValue(),
                                          /*cutoff=*/test::DistantPast()));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesWithAllEntriesWithinDuration) {
  // Arrange
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue()
                      .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                      .Set("value", 3.0))
          .Append(
              base::DictValue()
                  .Set("day", test::DistantPast().InSecondsFSinceUnixEpoch())
                  .Set("value", 5.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(8.0, SumTimePeriodStorageListValues(
                            list, /*cutoff=*/test::DistantPast()));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesWithEntriesWithinDuration) {
  // Arrange
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue()
                      .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                      .Set("value", 3.0))
          .Append(
              base::DictValue()
                  .Set("day", test::DistantPast().InSecondsFSinceUnixEpoch())
                  .Set("value", 42.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(3.0, SumTimePeriodStorageListValues(
                            list, /*cutoff=*/test::Now() - base::Days(7)));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesWithNoEntriesWithinDuration) {
  // Arrange
  const base::ListValue list =
      base::ListValue()
          .Append(
              base::DictValue()
                  .Set("day", test::DistantPast().InSecondsFSinceUnixEpoch())
                  .Set("value", 3.0))
          .Append(
              base::DictValue()
                  .Set("day", test::DistantPast().InSecondsFSinceUnixEpoch())
                  .Set("value", 5.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, SumTimePeriodStorageListValues(
                            list, /*cutoff=*/test::Now() - base::Days(7)));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesWithEntryOnCusp) {
  // Arrange
  const base::Time cutoff = test::Now() - base::Days(7);
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue()
                      .Set("day", cutoff.InSecondsFSinceUnixEpoch())
                      .Set("value", 7.0))
          .Append(base::DictValue()
                      .Set("day", (cutoff - base::Milliseconds(1))
                                      .InSecondsFSinceUnixEpoch())
                      .Set("value", 42.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(7.0,
                   SumTimePeriodStorageListValues(list, /*cutoff=*/cutoff));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesIgnoresNonDictEntries) {
  // Arrange
  const base::ListValue list =
      base::ListValue().Append("foo").Append(42).Append(
          base::DictValue()
              .Set("day", test::Now().InSecondsFSinceUnixEpoch())
              .Set("value", 7.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(7.0, SumTimePeriodStorageListValues(
                            list, /*cutoff=*/test::DistantPast()));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesIgnoresEntriesWithoutDayField) {
  // Arrange
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue().Set("value", 42.0))
          .Append(base::DictValue()
                      .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                      .Set("value", 7.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(7.0, SumTimePeriodStorageListValues(
                            list, /*cutoff=*/test::DistantPast()));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesIgnoresEntriesWithoutValueField) {
  // Arrange
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue().Set("day",
                                        test::Now().InSecondsFSinceUnixEpoch()))
          .Append(base::DictValue()
                      .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                      .Set("value", 7.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(7.0, SumTimePeriodStorageListValues(
                            list, /*cutoff=*/test::DistantPast()));
}

TEST_F(BraveAdsConditionMatcherTimePeriodStorageUtilTest,
       SumListValuesForMultipleEntriesWithinDuration) {
  // Arrange
  const base::ListValue list =
      base::ListValue()
          .Append(base::DictValue()
                      .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                      .Set("value", 1.0))
          .Append(base::DictValue()
                      .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                      .Set("value", 2.0))
          .Append(base::DictValue()
                      .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                      .Set("value", 3.0));

  // Act & Assert
  EXPECT_DOUBLE_EQ(6.0, SumTimePeriodStorageListValues(
                            list, /*cutoff=*/test::DistantPast()));
}

}  // namespace brave_ads
