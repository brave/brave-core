/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_MAC_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_MAC_H_

#include <memory>

#include "base/feature_list.h"
#include "brave/browser/upgrade_when_idle/upgrade_when_idle.h"
#include "chrome/browser/chrome_browser_main_mac.h"

namespace brave {
BASE_DECLARE_FEATURE(kUpgradeWhenIdle);
}

class BraveBrowserMainPartsMac : public ChromeBrowserMainPartsMac {
 public:
  BraveBrowserMainPartsMac(bool is_integration_test, StartupData* startup_data);
  ~BraveBrowserMainPartsMac() override;

 private:
  // ChromeBrowserMainPartsMac overrides:
  void PreCreateMainMessageLoop() override;
  void PostProfileInit(Profile* profile, bool is_initial_profile) override;
  void PostMainMessageLoopRun() override;

  std::unique_ptr<brave::UpgradeWhenIdle> upgrade_when_idle_;
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_MAC_H_
