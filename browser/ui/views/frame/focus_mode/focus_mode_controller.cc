/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode/focus_mode_controller.h"

#include "base/check_deref.h"

FocusModeController::FocusModeController(Delegate* delegate,
                                         views::Widget* widget)
    : delegate_(CHECK_DEREF(delegate)), widget_(CHECK_DEREF(widget)) {}

FocusModeController::~FocusModeController() = default;

void FocusModeController::SetEnabled(bool enabled) {
  if (enabled_ == enabled) {
    return;
  }
  enabled_ = enabled;
  delegate_->OnFocusModeChanged(enabled_);
}

bool FocusModeController::IsEnabled() const {
  return enabled_;
}
