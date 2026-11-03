/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

DEFINE_USER_DATA(FocusModeController);

FocusModeController::FocusModeController(ui::UnownedUserDataHost& host)
    : scoped_unowned_user_data_(host, *this) {}

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
  return enabled_ && suppress_count_ == 0;
}

void FocusModeController::SetEnabled(bool enabled) {
  if (enabled_ == enabled) {
    return;
  }
  const bool was_enabled = IsEnabled();
  enabled_ = enabled;
  if (IsEnabled() != was_enabled) {
    observers_.Notify(&Observer::OnFocusModeToggled, IsEnabled());
  }
}

void FocusModeController::ToggleEnabled() {
  SetEnabled(!enabled_);
}

base::ScopedClosureRunner FocusModeController::Suppress() {
  const bool was_enabled = IsEnabled();
  ++suppress_count_;
  if (was_enabled) {
    observers_.Notify(&Observer::OnFocusModeToggled, false);
  }
  return base::ScopedClosureRunner(base::BindOnce(
      &FocusModeController::EndSuppress, weak_factory_.GetWeakPtr()));
}

void FocusModeController::EndSuppress() {
  CHECK_GT(suppress_count_, 0);
  --suppress_count_;
  if (IsEnabled()) {
    observers_.Notify(&Observer::OnFocusModeToggled, true);
  }
}
