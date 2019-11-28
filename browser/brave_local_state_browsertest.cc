/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/in_process_browser_test.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/common/tor/pref_names.h"
#endif

using BraveLocalStateBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, BasicTest) {
#if BUILDFLAG(ENABLE_TOR)
    // Tor is enabled by default.
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabled());
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabledAtNextLaunching());
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabledChanged());
#endif
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, PRE_TorDisabledTest) {
  // Set to same value(enable) to next launching prefs, doesn't affect anything.
  tor::TorProfileService::SetTorDisabledAtNextLaunching(false);
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabled());
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabledAtNextLaunching());
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabledChanged());

  // Check setting to disable to next launching prefs doesn't affect
  // IsTorDisabled().
  tor::TorProfileService::SetTorDisabledAtNextLaunching(true);
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabled());
  EXPECT_TRUE(tor::TorProfileService::IsTorDisabledAtNextLaunching());
  EXPECT_TRUE(tor::TorProfileService::IsTorDisabledChanged());
}

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, TorDisabledTest) {
  // Check IsTorDisabled() is changed from previous test run.
  EXPECT_TRUE(tor::TorProfileService::IsTorDisabled());
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabledAtNextLaunching());
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabledChanged());
}
#endif
