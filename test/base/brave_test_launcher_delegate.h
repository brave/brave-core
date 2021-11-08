/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_TEST_BASE_BRAVE_TEST_LAUNCHER_DELEGATE_H_
#define BRAVE_TEST_BASE_BRAVE_TEST_LAUNCHER_DELEGATE_H_

#include "chrome/test/base/chrome_test_launcher.h"

class BraveTestLauncherDelegate : public ChromeTestLauncherDelegate {
 public:
  // Does not take ownership of ChromeTestSuiteRunner.
  explicit BraveTestLauncherDelegate(ChromeTestSuiteRunner* runner);
  BraveTestLauncherDelegate(const BraveTestLauncherDelegate&) = delete;
  BraveTestLauncherDelegate& operator=(const BraveTestLauncherDelegate&) =
      delete;
  ~BraveTestLauncherDelegate() override;

 private:
  // ChromeLauncherDelegate:
  content::ContentMainDelegate* CreateContentMainDelegate() override;
};

#endif  // BRAVE_TEST_BASE_BRAVE_TEST_LAUNCHER_DELEGATE_H_
