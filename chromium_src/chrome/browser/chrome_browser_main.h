/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_H_

#define ChromeBrowserMainParts ChromeBrowserMainParts_ChromiumImpl
#include "src/chrome/browser/chrome_browser_main.h"  // IWYU pragma: export
#undef ChromeBrowserMainParts

class ChromeBrowserMainParts : public ChromeBrowserMainParts_ChromiumImpl {
 public:
  ChromeBrowserMainParts(bool is_integration_test, StartupData* startup_data);

  ChromeBrowserMainParts(const ChromeBrowserMainParts&) = delete;
  ChromeBrowserMainParts& operator=(const ChromeBrowserMainParts&) = delete;
  ~ChromeBrowserMainParts() override;

  int PreMainMessageLoopRun() override;
  void PreBrowserStart() override;
  void PostBrowserStart() override;
  void PreShutdown() override;
  void PreProfileInit() override;
  void PostProfileInit(Profile* profile, bool is_initial_profile) override;

 private:
  friend class ChromeBrowserMainExtraPartsTor;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CHROME_BROWSER_MAIN_H_
