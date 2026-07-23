/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.h"
#include "chrome/browser/profiles/pref_service_builder_utils.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class BraveBrowserPrefsTest : public testing::Test {
 protected:
  BraveBrowserPrefsTest() {
    // Some migration code relies on prefs registered by keyed service
    // factories, so build them before registering profile prefs. Mirrors
    // upstream chrome/browser/prefs/browser_prefs_unittest.cc.
    ChromeBrowserMainExtraPartsProfiles::
        EnsureBrowserContextKeyedServiceFactoriesBuilt();
    RegisterProfilePrefs(/*is_signin_profile=*/false,
                         g_browser_process->GetApplicationLocale(),
                         prefs_.registry());
  }

  void MigrateObsoletePrefs() {
    MigrateObsoleteProfilePrefs(&prefs_, base::FilePath());
  }

  // The task environment is needed because some keyed services CHECK for
  // things like content::BrowserThread::CurrentlyOn(BrowserThread::UI).
  content::BrowserTaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

// prefs::kTabSearchPinnedToTabstrip is desktop only prefs.
#if !BUILDFLAG(IS_ANDROID)

// When the deprecated pref was never set by the user, the migration must not
// touch the upstream pref, leaving it at its default (true).
TEST_F(BraveBrowserPrefsTest, TabSearchShow_Untouched_KeepsUpstreamDefault) {
  ASSERT_TRUE(prefs_.FindPreference(kTabsSearchShow)->IsDefaultValue());

  MigrateObsoletePrefs();

  EXPECT_TRUE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));
  EXPECT_TRUE(prefs_.FindPreference(kTabsSearchShow)->IsDefaultValue());
}

// An explicit `true` is carried over to the upstream pref.
TEST_F(BraveBrowserPrefsTest, TabSearchShow_ExplicitTrue_Migrates) {
  prefs_.SetBoolean(kTabsSearchShow, true);

  MigrateObsoletePrefs();

  EXPECT_TRUE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));
  EXPECT_TRUE(prefs_.FindPreference(kTabsSearchShow)->IsDefaultValue());
}

// An explicit `false` overrides the upstream default of `true`. This is the
// case that proves the migration carries a non-default user choice across.
TEST_F(BraveBrowserPrefsTest, TabSearchShow_ExplicitFalse_Migrates) {
  prefs_.SetBoolean(kTabsSearchShow, false);
  ASSERT_TRUE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));

  MigrateObsoletePrefs();

  EXPECT_FALSE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));
  EXPECT_TRUE(prefs_.FindPreference(kTabsSearchShow)->IsDefaultValue());
}

#endif

}  // namespace
