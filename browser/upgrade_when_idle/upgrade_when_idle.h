// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UPGRADE_WHEN_IDLE_UPGRADE_WHEN_IDLE_H_
#define BRAVE_BROWSER_UPGRADE_WHEN_IDLE_UPGRADE_WHEN_IDLE_H_

#include <utility>

#include "base/functional/callback_forward.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile_manager.h"
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
  explicit UpgradeWhenIdle(ProfileManager* profile_manager);
  ~UpgradeWhenIdle() override;

  // UpgradeObserver:
  void OnUpgradeRecommended() override;

  void SetCheckIdleCallbackForTesting(base::OnceClosure callback) {
    check_idle_callback_for_testing_ = std::move(callback);
  }

 private:
  void CheckIdle();
  bool CanRelaunch();
  bool AreAnyBrowsersOpen();
  bool AreAnyClearDataOnExitSettingsEnabled();
  bool AttemptRelaunch();

  // Can be overridden for testing.
  virtual size_t GetBrowserWindowCount();

  raw_ptr<ProfileManager> profile_manager_ = nullptr;

  // Timer for periodic idle checks.
  base::RepeatingTimer idle_check_timer_;

  bool is_relaunching_ = false;

  SEQUENCE_CHECKER(sequence_checker_);

  base::OnceClosure check_idle_callback_for_testing_;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UPGRADE_WHEN_IDLE_UPGRADE_WHEN_IDLE_H_
