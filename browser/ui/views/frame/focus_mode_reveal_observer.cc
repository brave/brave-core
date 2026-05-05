/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_reveal_observer.h"

#include <utility>

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

FocusModeRevealObserver::FocusModeRevealObserver(
    BraveBrowserView* browser_view,
    FractionCallback on_fraction_changed,
    ToggledCallback on_toggled)
    : browser_view_(browser_view),
      on_fraction_changed_(std::move(on_fraction_changed)),
      on_toggled_(std::move(on_toggled)) {
  if (auto* controller =
          browser_view_->browser()->GetFeatures().focus_mode_controller()) {
    observation_.Observe(controller);
  }
}

FocusModeRevealObserver::~FocusModeRevealObserver() = default;

void FocusModeRevealObserver::OnFocusModeToggled(bool enabled) {
  if (on_toggled_) {
    on_toggled_.Run(enabled);
  }

  auto* overlay = browser_view_->focus_mode_top_overlay();
  if (!overlay || !enabled) {
    reveal_subscription_ = {};
    on_fraction_changed_.Run(1.0);
    return;
  }

  reveal_subscription_ =
      overlay->AddRevealFractionChangedCallback(on_fraction_changed_);
  on_fraction_changed_.Run(overlay->GetRevealFraction());
}
