/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode/focus_mode_controller.h"

#include <utility>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/views/frame/focus_mode/focus_mode_reveal_zone.h"
#include "brave/browser/ui/views/frame/focus_mode/focus_mode_revealed_lock.h"

FocusModeController::FocusModeController(Delegate* delegate,
                                         views::Widget* widget)
    : delegate_(CHECK_DEREF(delegate)), widget_(CHECK_DEREF(widget)) {
  for (auto edge : {Edge::kTop, Edge::kLeft, Edge::kRight}) {
    zones_[edge] = std::make_unique<FocusModeRevealZone>(
        base::BindRepeating(&FocusModeController::OnRevealFractionChanged,
                            base::Unretained(this), edge));
  }
}

FocusModeController::~FocusModeController() = default;

void FocusModeController::SetEnabled(bool enabled) {
  if (enabled_ == enabled) {
    return;
  }
  enabled_ = enabled;
  if (!enabled_) {
    for (auto& [edge, zone] : zones_) {
      zone->Reset();
    }
  }
  delegate_->OnFocusModeChanged(enabled_);
}

bool FocusModeController::IsEnabled() const {
  return enabled_;
}

bool FocusModeController::IsRevealed(Edge edge) const {
  auto* zone = GetZone(edge);
  return zone && enabled_ &&
         zone->state() != FocusModeRevealZone::State::kClosed;
}

double FocusModeController::GetVisibleFraction(Edge edge) const {
  auto* zone = GetZone(edge);
  return zone ? zone->visible_fraction() : 0.0;
}

std::unique_ptr<FocusModeRevealedLock> FocusModeController::GetRevealedLock(
    Edge edge,
    FocusModeTransition transition) {
  if (!enabled_) {
    return nullptr;
  }
  auto* zone = GetZone(edge);
  CHECK(zone);
  return std::make_unique<FocusModeRevealedLock>(zone->GetWeakPtr(),
                                                 transition);
}

void FocusModeController::OnRevealFractionChanged(Edge edge, double fraction) {
  delegate_->OnFocusModeRevealFractionChanged(edge, fraction);
}

FocusModeRevealZone* FocusModeController::GetZone(Edge edge) const {
  auto it = zones_.find(edge);
  return it != zones_.end() ? it->second.get() : nullptr;
}
