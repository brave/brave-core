/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal/edge_hover_detector.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/types/event_type.h"
#include "ui/views/event_monitor.h"

namespace {

constexpr base::TimeDelta kMouseEnterDelay = base::Milliseconds(60);
constexpr base::TimeDelta kMouseExitDelay = base::Milliseconds(300);

}  // namespace

EdgeHoverDetector::EdgeHoverDetector(
    views::Widget* host,
    IsHoverPointCallback is_hover_point,
    HoverStateChangedCallback hover_state_changed)
    : host_(host),
      is_hover_point_(std::move(is_hover_point)),
      hover_state_changed_(std::move(hover_state_changed)) {
  CHECK(host_);
  CHECK(is_hover_point_);
  CHECK(hover_state_changed_);

  host_observation_.Observe(host_);

  event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      this, host_->GetNativeWindow(),
      {ui::EventType::kMouseMoved, ui::EventType::kMouseExited});
}

EdgeHoverDetector::~EdgeHoverDetector() = default;

void EdgeHoverDetector::Update() {
  if (!host_) {
    return;
  }
  const gfx::Point cursor = display::Screen::Get()->GetCursorScreenPoint();
  ApplyWithDelay(is_hover_point_.Run(cursor));
}

void EdgeHoverDetector::OnWidgetDestroying(views::Widget* widget) {
  event_monitor_.reset();
  timer_.Stop();
  host_observation_.Reset();
  host_ = nullptr;
}

void EdgeHoverDetector::OnEvent(const ui::Event& event) {
  if (event.type() == ui::EventType::kMouseExited) {
    ApplyWithDelay(false);
  } else if (event.type() == ui::EventType::kMouseMoved) {
    Update();
  }
}

void EdgeHoverDetector::ApplyWithDelay(bool hovering) {
  if (hovering == hovering_) {
    timer_.Stop();
    return;
  }
  if (timer_.IsRunning()) {
    return;
  }
  timer_.Start(FROM_HERE, hovering ? kMouseEnterDelay : kMouseExitDelay,
               base::BindOnce(&EdgeHoverDetector::OnTimerFired,
                              base::Unretained(this), hovering));
}

void EdgeHoverDetector::OnTimerFired(bool hovering) {
  if (hovering_ == hovering) {
    return;
  }
  hovering_ = hovering;
  hover_state_changed_.Run(hovering_);
}
