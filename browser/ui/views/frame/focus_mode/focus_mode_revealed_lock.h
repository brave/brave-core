/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_REVEALED_LOCK_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_REVEALED_LOCK_H_

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/frame/focus_mode/focus_mode_types.h"

class FocusModeRevealZone;

// RAII handle that keeps a reveal zone open while alive. Acquiring the lock
// increments the zone's ref-count and triggers a reveal; destroying it
// decrements the count and may trigger a hide. Safe to outlive the zone.
class FocusModeRevealedLock {
 public:
  FocusModeRevealedLock(base::WeakPtr<FocusModeRevealZone> zone,
                        FocusModeTransition transition);
  ~FocusModeRevealedLock();

  FocusModeRevealedLock(const FocusModeRevealedLock&) = delete;
  FocusModeRevealedLock& operator=(const FocusModeRevealedLock&) = delete;

 private:
  base::WeakPtr<FocusModeRevealZone> zone_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_REVEALED_LOCK_H_
