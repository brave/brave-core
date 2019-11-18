/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "third_party/widevine/cdm/buildflags.h"

using WidevinePrefsMigrationTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(WidevinePrefsMigrationTest, PrefMigrationTest) {
  // After migration, local state doesn't have default value.
  EXPECT_FALSE(g_browser_process->local_state()->
      FindPreference(kWidevineOptedIn)->IsDefaultValue());

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  EXPECT_FALSE(g_browser_process->local_state()->
      FindPreference(kWidevineInstalledVersion)->IsDefaultValue());
#endif
}
