/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
#define BRAVE_APP_BRAVE_MAIN_DELEGATE_H_

#include <optional>

#include "base/gtest_prod_util.h"
#include "chrome/app/chrome_main_delegate.h"

class BraveMainDelegateUnitTest;

// Chrome implementation of ContentMainDelegate.
class BraveMainDelegate : public ChromeMainDelegate {
 public:
  BraveMainDelegate(const BraveMainDelegate&) = delete;
  BraveMainDelegate& operator=(const BraveMainDelegate&) = delete;
#if BUILDFLAG(IS_ANDROID)
  BraveMainDelegate();
#endif

  // `timestamps.exe_entry_point_ticks` is the time at which the main function
  // of the executable was entered. On Windows, StartupTimestamps contains
  // timing information for calls to base::PreReadFile. `timestamps`' lifetime
  // does not need to last beyond the constructor call.
  explicit BraveMainDelegate(const StartupTimestamps& timestamps);
  ~BraveMainDelegate() override;

 protected:
  // content::ContentMainDelegate implementation:
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  content::ContentUtilityClient* CreateContentUtilityClient() override;
  std::optional<int> BasicStartupComplete() override;
  void PreSandboxStartup() override;
  std::optional<int> PostEarlyInitialization(
      ChromeMainDelegate::InvokedIn invoked_in) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveMainDelegateUnitTest,
                           DefaultCommandLineOverrides);
  FRIEND_TEST_ALL_PREFIXES(BraveMainDelegateUnitTest,
                           OverrideSwitchFromCommandLine);

  static void AppendCommandLineOptions();
};

#endif  // BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
