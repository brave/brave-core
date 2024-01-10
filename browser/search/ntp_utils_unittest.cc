// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/search/ntp_utils.h"

#include <memory>

#include "brave/components/constants/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class NTPUtilsTest : public ::testing::Test {
 public:
  NTPUtilsTest() = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
  }

  Profile* profile() { return profile_.get(); }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

 protected:
  // BrowserTaskEnvironment is needed before TestingProfile
  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(NTPUtilsTest, MigratesHideWidgetTrue) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  auto* prefs = GetPrefs();
  prefs->ClearPref(kNewTabPageHideAllWidgets);
  prefs->ClearPref(kNewTabPageShowRewards);
  prefs->ClearPref(kNewTabPageShowBraveTalk);
  // Manually turn all off
  prefs->SetBoolean(kNewTabPageShowRewards, false);
  prefs->SetBoolean(kNewTabPageShowBraveTalk, false);
  // Migrate
  new_tab_page::MigrateNewTabPagePrefs(GetPrefs());
  // Expect migrated to off
  EXPECT_TRUE(prefs->GetBoolean(kNewTabPageHideAllWidgets));
}

TEST_F(NTPUtilsTest, MigratesHideWidgetFalse) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  auto* prefs = GetPrefs();
  prefs->ClearPref(kNewTabPageHideAllWidgets);
  prefs->ClearPref(kNewTabPageShowRewards);
  prefs->ClearPref(kNewTabPageShowBraveTalk);
  // Manually turn some off
  prefs->SetBoolean(kNewTabPageShowRewards, false);
  prefs->SetBoolean(kNewTabPageShowBraveTalk, true);
  // Migrate
  new_tab_page::MigrateNewTabPagePrefs(GetPrefs());
  // Expect not migrated
  EXPECT_FALSE(prefs->GetBoolean(kNewTabPageHideAllWidgets));
}

TEST_F(NTPUtilsTest, MigratesHideWidgetFalseDefault) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  auto* prefs = GetPrefs();
  prefs->ClearPref(kNewTabPageHideAllWidgets);
  prefs->ClearPref(kNewTabPageShowRewards);
  prefs->ClearPref(kNewTabPageShowBraveTalk);
  // Don't manually change any settings and expect not migrated
  EXPECT_FALSE(prefs->GetBoolean(kNewTabPageHideAllWidgets));
}
