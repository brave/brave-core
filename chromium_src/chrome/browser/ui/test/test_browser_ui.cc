/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/test/test_browser_ui.h"

#include "testing/gtest/include/gtest/gtest.h"

// A workaround for Brave CIs.
// TestBrowserUi don't want to run when the system is set to use dark theme on
// Windows. If dark theme is detected Chromium code calls GTEST_SKIP in
// ShowAndVerifyUi. Our CIs are set up with light theme, but occasionally tests
// derived from TestBrowserUi hit GTEST_SKIP. The only reason this could happen
// is if another test that runs in parallel switches the system to the dark
// theme, or some prior test messes up the system and leaves it in dark theme
// mode permanently. The problem is that GTEST_SKIP doesn't appear to fully
// exit the test. Instead it skips the rest of the ShowAndVerifyUi method and
// then returns and the test continues (and then typically fails because of the
// later test expectations). Upstream comment says the light theme is needed
// because "Gold files for pixel tests are for light mode", however, it doesn't
// appear that all (any?) tests that derive from TestBrowserUi actually use
// pixel tests. Those that don't care can continue running and succeed. Those
// that do care would fail, which is no worse than what we have with how
// GTEST_SKIP works.
#undef GTEST_SKIP
#define GTEST_SKIP() \
  LOG(WARNING) << "Brave: forcing test to run. Original Chromium behavior: "

#include "src/chrome/browser/ui/test/test_browser_ui.cc"
#undef GTEST_SKIP
