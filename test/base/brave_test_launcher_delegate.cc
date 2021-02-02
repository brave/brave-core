/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_test_launcher_delegate.h"

#include "brave/app/brave_main_delegate.h"

#if defined(OS_MAC) || defined(OS_LINUX)
#include "chrome/browser/first_run/first_run_internal.h"
#endif

BraveTestLauncherDelegate::BraveTestLauncherDelegate(
    ChromeTestSuiteRunner* runner)
    : ChromeTestLauncherDelegate(runner) {
  // Suppress first run dialg during the test. It can cause some tests timeout.
  // It's not used on Windows.
#if defined(OS_MAC) || defined(OS_LINUX)
  first_run::internal::ForceFirstRunDialogShownForTesting(false);
#endif
}

BraveTestLauncherDelegate::~BraveTestLauncherDelegate() = default;

content::ContentMainDelegate*
BraveTestLauncherDelegate::CreateContentMainDelegate() {
  return new BraveMainDelegate(base::TimeTicks::Now());
}
