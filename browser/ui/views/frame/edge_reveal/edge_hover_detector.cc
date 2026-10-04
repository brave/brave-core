/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal/edge_hover_detector.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/types/event_type.h"
#include "ui/views/event_monitor.h"

EdgeHoverDetector::EdgeHoverDetector(
    views::Widget* host,
    IsHoverPointCallback is_hover_point,
    HoverStateChangedCallback hover_state_changed)
    : is_hover_point_(std::move(is_hover_point)),
      hover_state_changed_(std::move(hover_state_changed)) {
  CHECK(host);
  CHECK(is_hover_point_);
  CHECK(hover_state_changed_);

  host_observation_.Observe(host);

  event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      this, host->GetNativeWindow(),
      {ui::EventType::kMouseMoved, ui::EventType::kMouseExited});
}

EdgeHoverDetector::~EdgeHoverDetector() = default;

void EdgeHoverDetector::DetectHoverState() {
  if (!host_observation_.IsObserving()) {
    return;
  }
  ApplyWithDelay(IsCursorHovering());
}

void EdgeHoverDetector::HandleEventForTesting(const ui::Event& event) {
  OnEvent(event);
}

void EdgeHoverDetector::OnWidgetDestroying(views::Widget* widget) {
  event_monitor_.reset();
  timer_.Stop();
  host_observation_.Reset();
}

void EdgeHoverDetector::OnEvent(const ui::Event& event) {
  if (event.type() == ui::EventType::kMouseExited) {
    ApplyWithDelay(false);
  } else if (event.type() == ui::EventType::kMouseMoved) {
    ApplyWithDelay(IsCursorHovering());
  }
}

bool EdgeHoverDetector::IsCursorHovering() {
  return is_hover_point_.Run(display::Screen::Get()->GetCursorScreenPoint());
}

void EdgeHoverDetector::ApplyWithDelay(bool hovering) {
  // If we are transitioning back to the current hover state (e.g. the mouse
  // exits the edge very quickly after entering it), then cancel the toggle
  // timer, if active.
  if (hovering == hovering_) {
    timer_.Stop();
    return;
  }
  // If the toggle timer is already active, then let it proceed.
  if (timer_.IsRunning()) {
    return;
  }
  // Start the toggle timer. When it fires, the hover state will be toggled.
  timer_.Start(FROM_HERE, hovering ? kMouseEnterDelay : kMouseExitDelay,
               base::BindOnce(&EdgeHoverDetector::ToggleHoverState,
                              base::Unretained(this)));
}

void EdgeHoverDetector::ToggleHoverState() {
  hovering_ = !hovering_;
  hover_state_changed_.Run(hovering_);
}
