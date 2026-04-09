/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode/focus_mode_revealed_lock.h"

#include "brave/browser/ui/views/frame/focus_mode/focus_mode_reveal_zone.h"

FocusModeRevealedLock::FocusModeRevealedLock(
    base::WeakPtr<FocusModeRevealZone> zone,
    FocusModeTransition transition)
    : zone_(std::move(zone)) {
  if (zone_) {
    zone_->LockRevealedState(transition);
  }
}

FocusModeRevealedLock::~FocusModeRevealedLock() {
  if (zone_) {
    zone_->UnlockRevealedState();
  }
}
