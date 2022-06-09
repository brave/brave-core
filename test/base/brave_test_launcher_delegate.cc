/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_test_launcher_delegate.h"

#include "brave/app/brave_main_delegate.h"
#include "brave/browser/ui/brave_browser.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "chrome/browser/first_run/first_run_internal.h"
#endif

BraveTestLauncherDelegate::BraveTestLauncherDelegate(
    ChromeTestSuiteRunner* runner)
    : ChromeTestLauncherDelegate(runner) {
  // Suppress first run dialg during the test. It can cause some tests timeout.
  // It's not used on Windows.
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  first_run::internal::ForceFirstRunDialogShownForTesting(false);
#endif

  // Suppress browser window closing dialog during the test.
  // It can cause some tests timeout.
  BraveBrowser::SuppressBrowserWindowClosingDialogForTesting(true);
}

BraveTestLauncherDelegate::~BraveTestLauncherDelegate() = default;

content::ContentMainDelegate*
BraveTestLauncherDelegate::CreateContentMainDelegate() {
  return new BraveMainDelegate(base::TimeTicks::Now());
}
