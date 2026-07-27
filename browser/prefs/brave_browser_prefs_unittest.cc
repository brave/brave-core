/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.h"
#include "chrome/browser/profiles/pref_service_builder_utils.h"
#include "chrome/browser/ui/toolbar/toolbar_pref_names.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// The tab search action is serialized into prefs::kPinnedActions as the
// stringized enum name of actions::kActionTabSearch. Hardcoded (rather than
// derived via actions::ActionIdMap, which isn't initialized in unit tests)
// because this migration matches the frozen on-disk value written by older
// builds, which must not change even if the upstream enum is later renamed.
constexpr char kTabSearchActionId[] = "kActionTabSearch";

// Upstream marker pref, set only for profiles that went through the era where
// the tab search action was auto-pinned as a toolbar action. Referenced by
// string because it is a deprecated, file-local pref upstream.
constexpr char kTabSearchMigrationCompletePref[] =
    "toolbar.tab_search_migration_complete";

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

  void SetPinnedActions(std::vector<std::string_view> action_ids) {
    base::ListValue pinned;
    for (const auto& action_id : action_ids) {
      pinned.Append(action_id);
    }
    prefs_.SetList(prefs::kPinnedActions, std::move(pinned));
  }

  // The task environment is needed because some keyed services CHECK for
  // things like content::BrowserThread::CurrentlyOn(BrowserThread::UI).
  content::BrowserTaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

// A profile that went through the toolbar-button era (marker set) with the tab
// search action still pinned migrates to "pinned" in the combo button pref.
TEST_F(BraveBrowserPrefsTest, TabSearchShow_MarkerSetAndPinned_Migrates) {
  prefs_.SetBoolean(kTabSearchMigrationCompletePref, true);
  SetPinnedActions({kTabSearchActionId});
  // Start from the opposite value so the assertion proves the migration wrote
  // it rather than relying on the upstream default (true).
  prefs_.SetBoolean(prefs::kTabSearchPinnedToTabstrip, false);

  MigrateObsoletePrefs();

  EXPECT_TRUE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));
}

// A profile that went through the era (marker set) but no longer has the tab
// search action pinned migrates to "not pinned", overriding the upstream
// default of true. This is the case that carries a user's "off" choice across.
TEST_F(BraveBrowserPrefsTest, TabSearchShow_MarkerSetAndNotPinned_Migrates) {
  prefs_.SetBoolean(kTabSearchMigrationCompletePref, true);
  // kPinnedActions does not contain the tab search action.
  SetPinnedActions({});
  ASSERT_TRUE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));

  MigrateObsoletePrefs();

  EXPECT_FALSE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));
}

// Without the marker (the profile never went through the era) the migration
// must not run at all — even if the tab search action happens to be pinned, the
// combo button pref keeps whatever value it had.
TEST_F(BraveBrowserPrefsTest, TabSearchShow_NoMarker_DoesNotMigrate) {
  ASSERT_TRUE(
      prefs_.FindPreference(kTabSearchMigrationCompletePref)->IsDefaultValue());
  SetPinnedActions({kTabSearchActionId});
  // If the migration wrongly ran it would set this to true (action is pinned).
  prefs_.SetBoolean(prefs::kTabSearchPinnedToTabstrip, false);

  MigrateObsoletePrefs();

  EXPECT_FALSE(prefs_.GetBoolean(prefs::kTabSearchPinnedToTabstrip));
}

// The dead brave.tabs_search_show pref is always cleared.
TEST_F(BraveBrowserPrefsTest, TabSearchShow_DeadPrefCleared) {
  prefs_.SetBoolean(kTabsSearchShow, true);
  ASSERT_FALSE(prefs_.FindPreference(kTabsSearchShow)->IsDefaultValue());

  MigrateObsoletePrefs();

  EXPECT_TRUE(prefs_.FindPreference(kTabsSearchShow)->IsDefaultValue());
}

}  // namespace
