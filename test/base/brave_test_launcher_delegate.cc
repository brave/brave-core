/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_test_launcher_delegate.h"

#include "brave/app/brave_main_delegate.h"

BraveTestLauncherDelegate::BraveTestLauncherDelegate(
    ChromeTestSuiteRunner* runner)
    : ChromeTestLauncherDelegate(runner) {}

BraveTestLauncherDelegate::~BraveTestLauncherDelegate() = default;

content::ContentMainDelegate*
BraveTestLauncherDelegate::CreateContentMainDelegate() {
  return new BraveMainDelegate(base::TimeTicks::Now());
}
