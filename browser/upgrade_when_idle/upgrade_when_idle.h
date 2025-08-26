// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UPGRADE_WHEN_IDLE_UPGRADE_WHEN_IDLE_H_
#define BRAVE_BROWSER_UPGRADE_WHEN_IDLE_UPGRADE_WHEN_IDLE_H_

#include "base/timer/timer.h"
#include "chrome/browser/upgrade_detector/upgrade_observer.h"

namespace brave {

// This class relaunches the browser to apply a pending update when certain
// conditions are met:
// - No windows are open.
// - The system is idle (= no user input for a while or the screen is locked).
// - The browser wouldn't clear any data on exit.
// The motivation is to make pending updates take effect sooner. This is
// especially important on macOS where closing the last browser window does not
// quit the browser, and thus also does not apply an update.
class UpgradeWhenIdle : public UpgradeObserver {
 public:
  UpgradeWhenIdle();
  ~UpgradeWhenIdle() override;

 private:
  // UpgradeObserver:
  void OnUpgradeRecommended() override;

  void CheckIdle();
  bool CanRelaunch();
  bool AttemptRelaunch();

  // Timer for periodic idle checks.
  base::RepeatingTimer idle_check_timer_;

  bool is_relaunching_ = false;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UPGRADE_WHEN_IDLE_UPGRADE_WHEN_IDLE_H_
