/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/focus_mode/focus_mode_prefs.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/prefs/pref_service.h"

DEFINE_USER_DATA(FocusModeController);

FocusModeController::FocusModeController(ui::UnownedUserDataHost& host,
                                         PrefService& prefs)
    : scoped_unowned_user_data_(host, *this), prefs_(prefs) {
  pref_change_registrar_.Init(&prefs);
  pref_change_registrar_.Add(
      focus_mode::kFocusModeEnabled,
      base::BindRepeating(&FocusModeController::OnPrefChanged,
                          base::Unretained(this)));
  last_notified_enabled_ = IsEnabled();
}

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
  return prefs_->GetBoolean(focus_mode::kFocusModeEnabled) &&
         suppress_count_ == 0;
}

void FocusModeController::SetEnabled(bool enabled) {
  prefs_->SetBoolean(focus_mode::kFocusModeEnabled, enabled);
}

void FocusModeController::ToggleEnabled() {
  SetEnabled(!prefs_->GetBoolean(focus_mode::kFocusModeEnabled));
}

base::ScopedClosureRunner FocusModeController::Suppress() {
  ++suppress_count_;
  NotifyIfChanged();
  return base::ScopedClosureRunner(base::BindOnce(
      &FocusModeController::EndSuppress, weak_factory_.GetWeakPtr()));
}

void FocusModeController::EndSuppress() {
  CHECK_GT(suppress_count_, 0);
  --suppress_count_;
  NotifyIfChanged();
}

void FocusModeController::OnPrefChanged() {
  NotifyIfChanged();
}

void FocusModeController::NotifyIfChanged() {
  const bool enabled = IsEnabled();
  if (enabled == last_notified_enabled_) {
    return;
  }
  last_notified_enabled_ = enabled;
  observers_.Notify(&Observer::OnFocusModeToggled, enabled);
}
