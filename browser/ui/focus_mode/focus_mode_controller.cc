/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/focus_mode/focus_mode_prefs.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/prefs/pref_service.h"

DEFINE_USER_DATA(FocusModeController);

FocusModeController::FocusModeController(BrowserWindowInterface& browser,
                                         PrefService& prefs)
    : enabled_(prefs.GetBoolean(focus_mode::kFocusModeLastWindowState)),
      did_become_active_subscription_(browser.RegisterDidBecomeActive(
          base::BindRepeating(&FocusModeController::OnDidBecomeActive,
                              base::Unretained(this)))),
      scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this),
      prefs_(prefs) {}

FocusModeController::~FocusModeController() = default;

// static
FocusModeController* FocusModeController::From(
    BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

// static
const FocusModeController* FocusModeController::From(
    const BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

void FocusModeController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FocusModeController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool FocusModeController::IsEnabled() const {
  return enabled_;
}

void FocusModeController::SetEnabled(bool enabled) {
  if (enabled_ == enabled) {
    return;
  }
  enabled_ = enabled;
  UpdatePref();
  observers_.Notify(&Observer::OnFocusModeToggled, enabled);
}

void FocusModeController::ToggleEnabled() {
  SetEnabled(!enabled_);
}

void FocusModeController::OnDidBecomeActive(BrowserWindowInterface* browser) {
  UpdatePref();
}

void FocusModeController::UpdatePref() {
  prefs_->SetBoolean(focus_mode::kFocusModeLastWindowState, enabled_);
}
