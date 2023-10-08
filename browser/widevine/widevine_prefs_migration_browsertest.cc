/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

namespace {
constexpr bool kWidevineEnabledTestValue = true;
}  // namespace

using WidevinePrefsMigrationTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(WidevinePrefsMigrationTest, PrefMigrationTest) {
  g_browser_process->local_state()->ClearPref(kWidevineEnabled);
  EXPECT_TRUE(g_browser_process->local_state()
                  ->FindPreference(kWidevineEnabled)
                  ->IsDefaultValue());

  // Set profile prefs explicitly for migration test.
  browser()->profile()->GetPrefs()->SetBoolean(kWidevineEnabled,
                                               kWidevineEnabledTestValue);

  // Migrate and check it's done properly with previous profile prefs value.
  MigrateWidevinePrefs(browser()->profile()->GetPrefs());
  EXPECT_FALSE(g_browser_process->local_state()
                   ->FindPreference(kWidevineEnabled)
                   ->IsDefaultValue());
  EXPECT_EQ(kWidevineEnabledTestValue,
            g_browser_process->local_state()->GetBoolean(kWidevineEnabled));
}
