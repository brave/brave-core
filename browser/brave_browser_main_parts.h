/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_H_

#include "chrome/browser/chrome_browser_main.h"

class BraveBrowserMainParts : public ChromeBrowserMainParts {
 public:
  using ChromeBrowserMainParts::ChromeBrowserMainParts;

  BraveBrowserMainParts(const BraveBrowserMainParts&) = delete;
  BraveBrowserMainParts& operator=(const BraveBrowserMainParts&) = delete;
  ~BraveBrowserMainParts() override = default;

  void PreBrowserStart() override;
  void PostBrowserStart() override;
  void PreShutdown() override;
  void PreProfileInit() override;
  void PostProfileInit(Profile* profile, bool is_initial_profile) override;

 private:
  friend class ChromeBrowserMainExtraPartsTor;
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_H_
