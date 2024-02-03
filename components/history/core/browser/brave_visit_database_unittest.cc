/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "components/history/core/browser/url_database.h"
#include "components/history/core/browser/visit_database.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using base::Time;

namespace history {

// Inspired by Chromium's VisitDatabaseTest.IsKnownToSync
class BraveVisitDatabaseTest : public PlatformTest,
                               public URLDatabase,
                               public VisitDatabase {
 public:
  BraveVisitDatabaseTest() {}

 private:
  // Test setup.
  void SetUp() override {
    PlatformTest::SetUp();

    EXPECT_TRUE(db_.OpenInMemory());

    // Initialize the tables for this test.
    CreateURLTable(false);
    CreateMainURLIndex();
    InitVisitTable();
  }
  void TearDown() override {
    db_.Close();
    PlatformTest::TearDown();
  }

  // Provided for URL/VisitDatabase.
  sql::Database& GetDB() override { return db_; }

  sql::Database db_;
};

TEST_F(BraveVisitDatabaseTest, BraveGetKnownToSyncCount) {
  // Insert three rows, VisitIDs 1, 2, and 3.
  for (VisitID i = 1; i <= 3; i++) {
    VisitRow original(i, Time::Now(), 23, ui::PageTransitionFromInt(0), 19,
                      false, 0);
    AddVisit(&original, SOURCE_BROWSED);
    ASSERT_EQ(i, original.visit_id);  // Verifies that we added 1, 2, and 3
  }

  int known_to_sync_count = 0;
  ASSERT_TRUE(GetKnownToSyncCount(&known_to_sync_count));
  EXPECT_EQ(known_to_sync_count, 0);

  // Set 2 and 3 to be `is_known_to_sync`.
  VisitRow visit2;
  ASSERT_TRUE(GetRowForVisit(2, &visit2));
  EXPECT_FALSE(visit2.is_known_to_sync);
  visit2.is_known_to_sync = true;
  ASSERT_TRUE(UpdateVisitRow(visit2));

  VisitRow visit3;
  ASSERT_TRUE(GetRowForVisit(3, &visit3));
  EXPECT_FALSE(visit3.is_known_to_sync);
  visit3.is_known_to_sync = true;
  ASSERT_TRUE(UpdateVisitRow(visit3));

  ASSERT_TRUE(GetKnownToSyncCount(&known_to_sync_count));
  EXPECT_EQ(known_to_sync_count, 2);

  // Now clear out all `is_known_to_sync` bits and verify that we still count
  // correctly.
  SetAllVisitsAsNotKnownToSync();

  ASSERT_TRUE(GetKnownToSyncCount(&known_to_sync_count));
  EXPECT_EQ(known_to_sync_count, 0);
}

}  // namespace history
