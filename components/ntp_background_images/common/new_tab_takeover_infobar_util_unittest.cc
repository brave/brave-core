/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/common/new_tab_takeover_infobar_util.h"

#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NewTabTakeoverInfobarUtilTest : public testing::Test {
 public:
  NewTabTakeoverInfobarUtilTest() {
    RegisterProfilePrefs(pref_service_.registry());
  }

  ~NewTabTakeoverInfobarUtilTest() override = default;

  PrefService* pref_service() { return &pref_service_; }

 private:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(NewTabTakeoverInfobarUtilTest, ShowNewTabTakeoverInfobarUntilThreshold) {
  for (int i = 0; i < GetNewTabTakeoverInfobarShowCountThreshold(); ++i) {
    EXPECT_TRUE(ShouldShowNewTabTakeoverInfobar(pref_service()));
    RecordNewTabTakeoverInfobarWasShown(pref_service());
  }

  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest, SuppressNewTabTakeoverInfobar) {
  RecordNewTabTakeoverInfobarWasShown(pref_service());
  EXPECT_TRUE(ShouldShowNewTabTakeoverInfobar(pref_service()));

  SuppressNewTabTakeoverInfobar(pref_service());
  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

}  // namespace ntp_background_images
