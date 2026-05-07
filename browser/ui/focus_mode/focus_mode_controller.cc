/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

#include "base/check_deref.h"

FocusModeController::FocusModeController() = default;

FocusModeController::~FocusModeController() = default;

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
  observers_.Notify(&Observer::OnFocusModeToggled, enabled);
}

void FocusModeController::ToggleEnabled() {
  SetEnabled(!enabled_);
}

ScopedFocusModeDisable::ScopedFocusModeDisable(FocusModeController* controller)
    : controller_(CHECK_DEREF(controller)),
      was_enabled_(controller->IsEnabled()) {
  controller_->SetEnabled(false);
}

ScopedFocusModeDisable::~ScopedFocusModeDisable() {
  if (was_enabled_) {
    controller_->SetEnabled(true);
  }
}
