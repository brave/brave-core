/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "components/sync/base/user_selectable_type.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_sync {
namespace p3a {

TEST(BraveSyncP3ATest, TestEnabledTypes) {
  using syncer::UserSelectableType;

  base::HistogramTester histogram_tester;

  RecordEnabledTypes(true, {});
  histogram_tester.ExpectBucketCount(kEnabledTypesHistogramName,
                                     EnabledTypesAnswer::kAllTypes, 1);
  RecordEnabledTypes(
      false, {UserSelectableType::kBookmarks, UserSelectableType::kHistory,
              UserSelectableType::kExtensions, UserSelectableType::kApps,
              UserSelectableType::kPasswords, UserSelectableType::kPreferences,
              UserSelectableType::kThemes, UserSelectableType::kTabs,
              UserSelectableType::kAutofill});
  histogram_tester.ExpectBucketCount(kEnabledTypesHistogramName,
                                     EnabledTypesAnswer::kAllTypes, 2);

  RecordEnabledTypes(false, {});
  histogram_tester.ExpectBucketCount(
      kEnabledTypesHistogramName, EnabledTypesAnswer::kEmptyOrBookmarksOnly, 1);
  RecordEnabledTypes(false, {UserSelectableType::kBookmarks});
  histogram_tester.ExpectBucketCount(
      kEnabledTypesHistogramName, EnabledTypesAnswer::kEmptyOrBookmarksOnly, 2);

  RecordEnabledTypes(
      false, {UserSelectableType::kBookmarks, UserSelectableType::kHistory});
  histogram_tester.ExpectBucketCount(
      kEnabledTypesHistogramName, EnabledTypesAnswer::kBookmarksAndHistory, 1);

  RecordEnabledTypes(false, {UserSelectableType::kBookmarks,
                             UserSelectableType::kPreferences});
  histogram_tester.ExpectBucketCount(
      kEnabledTypesHistogramName,
      EnabledTypesAnswer::kMoreThanBookmarksAndHistory, 1);
  RecordEnabledTypes(
      false, {UserSelectableType::kBookmarks, UserSelectableType::kHistory,
              UserSelectableType::kApps});
  histogram_tester.ExpectBucketCount(
      kEnabledTypesHistogramName,
      EnabledTypesAnswer::kMoreThanBookmarksAndHistory, 2);
}

TEST(BraveSyncP3ATest, TestSyncedObjectsCount) {
  base::HistogramTester histogram_tester;

  RecordSyncedObjectsCount(0);
  histogram_tester.ExpectBucketCount(kSyncedObjectsCountHistogramName, 0, 1);
  RecordSyncedObjectsCount(1000);
  histogram_tester.ExpectBucketCount(kSyncedObjectsCountHistogramName, 0, 2);

  RecordSyncedObjectsCount(1001);
  histogram_tester.ExpectBucketCount(kSyncedObjectsCountHistogramName, 1, 1);
  RecordSyncedObjectsCount(10000);
  histogram_tester.ExpectBucketCount(kSyncedObjectsCountHistogramName, 1, 2);

  RecordSyncedObjectsCount(10001);
  histogram_tester.ExpectBucketCount(kSyncedObjectsCountHistogramName, 2, 1);
  RecordSyncedObjectsCount(49000);
  histogram_tester.ExpectBucketCount(kSyncedObjectsCountHistogramName, 2, 2);

  RecordSyncedObjectsCount(49001);
  histogram_tester.ExpectBucketCount(kSyncedObjectsCountHistogramName, 3, 1);
}

}  // namespace p3a
}  // namespace brave_sync
