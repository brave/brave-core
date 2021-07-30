// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/search/ntp_utils.h"

#include <memory>

#include "brave/common/pref_names.h"
#include "brave/components/crypto_dot_com/browser/buildflags/buildflags.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
#include "brave/components/crypto_dot_com/common/pref_names.h"
#endif

class NTPUtilsTest : public ::testing::Test {
 public:
  NTPUtilsTest() = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
  }

  Profile* profile() { return profile_.get(); }

 protected:
  // BrowserTaskEnvironment is needed before TestingProfile
  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(NTPUtilsTest, MigratesHideWidgetTrue) {
  // Manually turn all off
  auto* prefs = profile()->GetPrefs();
  prefs->SetBoolean(kNewTabPageShowRewards, false);
  prefs->SetBoolean(kNewTabPageShowTogether, false);
  prefs->SetBoolean(kNewTabPageShowBinance, false);
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  prefs->SetBoolean(kCryptoDotComNewTabPageShowCryptoDotCom, false);
#endif
  prefs->SetBoolean(kNewTabPageShowGemini, false);
  // Migrate
  new_tab_page::MigrateNewTabPagePrefs(profile());
  // Expect migrated to off
  EXPECT_TRUE(prefs->GetBoolean(kNewTabPageHideAllWidgets));
}

TEST_F(NTPUtilsTest, MigratesHideWidgetFalse) {
  // Manually turn some off
  auto* prefs = profile()->GetPrefs();
  prefs->SetBoolean(kNewTabPageShowRewards, false);
  prefs->SetBoolean(kNewTabPageShowTogether, true);
  prefs->SetBoolean(kNewTabPageShowBinance, false);
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  prefs->SetBoolean(kCryptoDotComNewTabPageShowCryptoDotCom, false);
#endif
  prefs->SetBoolean(kNewTabPageShowGemini, false);
  // Migrate
  new_tab_page::MigrateNewTabPagePrefs(profile());
  // Expect not migrated
  EXPECT_FALSE(prefs->GetBoolean(kNewTabPageHideAllWidgets));
}

TEST_F(NTPUtilsTest, MigratesHideWidgetFalseDefault) {
  // Don't manually change any settings
  // Migrate
  new_tab_page::MigrateNewTabPagePrefs(profile());
  // Expect not migrated
  auto* prefs = profile()->GetPrefs();
  EXPECT_FALSE(prefs->GetBoolean(kNewTabPageHideAllWidgets));
}
