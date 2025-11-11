// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/upgrade_when_idle/upgrade_when_idle.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/command_line.h"
#include "base/time/time.h"
#include "brave/browser/browsing_data/brave_clear_browsing_data.h"
#include "chrome/browser/first_run/upgrade_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/upgrade_detector/upgrade_detector.h"
#include "chrome/common/chrome_switches.h"
#include "components/prefs/pref_service.h"
#include "ui/base/idle/idle.h"

namespace {

// How much idle time (since last input event) must have passed until we
// restart the browser when an update is available and no window is open.
constexpr int kIdleAmount = 3;  // Minutes (or seconds, if testing).

bool IsTesting() {
  // Implementation similar to IsTesting() in upgrade_detector_impl.cc.
  const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  return cmd_line.HasSwitch(switches::kSimulateUpgrade) ||
         cmd_line.HasSwitch(switches::kCheckForUpdateIntervalSec) ||
         cmd_line.HasSwitch(switches::kSimulateCriticalUpdate) ||
         cmd_line.HasSwitch(switches::kSimulateOutdated) ||
         cmd_line.HasSwitch(switches::kSimulateOutdatedNoAU);
}

}  // namespace

namespace brave {

UpgradeWhenIdle::UpgradeWhenIdle(ProfileManager* profile_manager) {
  if (!profile_manager) {
    CHECK_IS_TEST();
  }
  profile_manager_ = profile_manager;
  UpgradeDetector::GetInstance()->AddObserver(this);
}

UpgradeWhenIdle::~UpgradeWhenIdle() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UpgradeDetector::GetInstance()->RemoveObserver(this);
}

void UpgradeWhenIdle::OnUpgradeRecommended() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // This function gets called repeatedly when an upgrade is available. When
  // testing, the interval is every 500ms. In that case, our idle timer needs to
  // have a shorter interval than that in order to run. We use 250ms.
  const base::TimeDelta idle_timer =
      IsTesting() ? base::Milliseconds(250) : base::Minutes(kIdleAmount);
  idle_check_timer_.Start(FROM_HERE, idle_timer, this,
                          &UpgradeWhenIdle::CheckIdle);
}

void UpgradeWhenIdle::CheckIdle() {
  // This function was inspired by UpgradeDetector::CheckIdle.
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (check_idle_callback_for_testing_) {
    std::move(check_idle_callback_for_testing_).Run();
  }

  if (!CanRelaunch()) {
    return;
  }

  int idle_time_allowed = IsTesting() ? kIdleAmount : kIdleAmount * 60;

  ui::IdleState state = ui::CalculateIdleState(idle_time_allowed);

  switch (state) {
    case ui::IDLE_STATE_LOCKED:
    case ui::IDLE_STATE_IDLE: {
      if (AttemptRelaunch()) {
        idle_check_timer_.Stop();
      }
      break;
    }
    case ui::IDLE_STATE_ACTIVE:
    case ui::IDLE_STATE_UNKNOWN:
      // Continue checking.
      break;
  }
}

bool UpgradeWhenIdle::CanRelaunch() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !AreAnyBrowsersOpen() && !AreAnyClearDataOnExitSettingsEnabled() &&
         !is_relaunching_;
}

bool UpgradeWhenIdle::AreAnyBrowsersOpen() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetBrowserWindowCount() > 0;
}

size_t UpgradeWhenIdle::GetBrowserWindowCount() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetAllBrowserWindowInterfaces().size();
}

bool UpgradeWhenIdle::AreAnyClearDataOnExitSettingsEnabled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!profile_manager_) {
    return false;
  }
  for (Profile* profile : profile_manager_->GetLoadedProfiles()) {
    if (content::BraveClearBrowsingData::IsClearOnExitEnabledForAnyType(
            profile)) {
      return true;
    }
  }
  return false;
}

bool UpgradeWhenIdle::AttemptRelaunch() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // We are relaunching the browser when there are no open windows.
  // Upstream's function chrome::AttemptRelaunch() opens the browser with a new
  // window, even when there were no open windows before. This function avoids
  // that shortcoming by appending the kNoStartupWindow switch.
  base::CommandLine new_cmd_line(*base::CommandLine::ForCurrentProcess());
  new_cmd_line.AppendSwitch(switches::kNoStartupWindow);
  is_relaunching_ = upgrade_util::RelaunchChromeBrowser(new_cmd_line);
  if (is_relaunching_) {
    chrome::AttemptExit();
  }
  return is_relaunching_;
}

}  // namespace brave
