/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_REVEAL_OBSERVER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_REVEAL_OBSERVER_H_

#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

class BraveBrowserView;

// Composition helper that wires a frame view (or other consumer) to focus
// mode's reveal animation. Owns the `FocusModeController` observation and the
// `FocusModeTopOverlay` reveal subscription, and routes per-fraction updates
// to a caller-supplied callback.
//
// On focus mode enable, the helper subscribes to the overlay's reveal callback
// and immediately invokes `on_fraction_changed` with the overlay's current
// reveal fraction (so consumers don't need a separate "prime" call). On
// disable, the helper drops the subscription and invokes `on_fraction_changed`
// with `1.0` (fully revealed = identity transform / fully visible), so
// consumers can use a single code path to reset state.
//
// If `on_toggled` is provided, it runs first on every toggle, before any
// fraction handling. Use it for side effects that aren't fraction-driven (e.g.
// title bar visibility updates).
//
// The helper is a no-op when the browser has no focus mode controller (e.g.
// non-normal browser types), or when the overlay isn't available.
class FocusModeRevealObserver : public FocusModeController::Observer {
 public:
  using FractionCallback = base::RepeatingCallback<void(double)>;
  using ToggledCallback = base::RepeatingCallback<void(bool)>;

  FocusModeRevealObserver(BraveBrowserView* browser_view,
                          FractionCallback on_fraction_changed,
                          ToggledCallback on_toggled = ToggledCallback());

  FocusModeRevealObserver(const FocusModeRevealObserver&) = delete;
  FocusModeRevealObserver& operator=(const FocusModeRevealObserver&) = delete;

  ~FocusModeRevealObserver() override;

  // FocusModeController::Observer:
  void OnFocusModeToggled(bool enabled) override;

 private:
  raw_ptr<BraveBrowserView> browser_view_;
  FractionCallback on_fraction_changed_;
  ToggledCallback on_toggled_;

  base::ScopedObservation<FocusModeController, FocusModeController::Observer>
      observation_{this};
  base::CallbackListSubscription reveal_subscription_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_REVEAL_OBSERVER_H_
