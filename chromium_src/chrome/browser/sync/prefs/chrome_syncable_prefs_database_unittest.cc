/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sync/prefs/chrome_syncable_prefs_database.h"

#include "components/sync/base/data_type.h"
#include "components/sync_preferences/common_syncable_prefs_database.h"
#include "components/sync_preferences/syncable_prefs_database.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveChromeSyncablePrefsDatabaseTest, BravePrefsAreSyncable) {
  ChromeSyncablePrefsDatabase db;

  const sync_preferences::SyncablePrefMetadata* shields =
      db.GetSyncablePrefMetadata(
          "profile.content_settings.exceptions.braveShields");
  ASSERT_TRUE(shields);
  EXPECT_EQ(shields->syncable_pref_id(), 300008);
  EXPECT_EQ(shields->data_type(), syncer::PREFERENCES);

  EXPECT_TRUE(db.IsPreferenceSyncable(
      "profile.content_settings.exceptions.braveSpeedreader"));
}

TEST(BraveChromeSyncablePrefsDatabaseTest, UpstreamPrefsStillSyncable) {
  ChromeSyncablePrefsDatabase db;
  EXPECT_TRUE(db.IsPreferenceSyncable(
      sync_preferences::kSyncablePriorityPrefForTesting));
  EXPECT_FALSE(db.IsPreferenceSyncable("brave.not_a_syncable_pref"));
}
