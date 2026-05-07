/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_HOVER_DETECTOR_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_HOVER_DETECTOR_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class EventMonitor;
}  // namespace views

// Detects when the mouse is "hovering" in a consumer-defined region of a host
// widget's window and emits a debounced bool signal suitable for driving
// auto-hide UI reveal.
class EdgeHoverDetector : public views::WidgetObserver,
                          public ui::EventObserver {
 public:
  using IsHoverPointCallback =
      base::RepeatingCallback<bool(const gfx::Point& screen_point)>;

  using HoverStateChangedCallback =
      base::RepeatingCallback<void(bool hovering)>;

  EdgeHoverDetector(views::Widget* host,
                    IsHoverPointCallback is_hover_point,
                    HoverStateChangedCallback hover_state_changed);

  EdgeHoverDetector(const EdgeHoverDetector&) = delete;
  EdgeHoverDetector& operator=(const EdgeHoverDetector&) = delete;
  ~EdgeHoverDetector() override;

  // Current committed hover state (post-debounce).
  bool hovering() const { return hovering_; }

  // Forces a re-evaluation using the current global cursor position.
  void Update();

 private:
  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  // ui::EventObserver:
  void OnEvent(const ui::Event& event) override;

  void ApplyWithDelay(bool hovering);
  void OnTimerFired(bool hovering);

  raw_ptr<views::Widget> host_;
  IsHoverPointCallback is_hover_point_;
  HoverStateChangedCallback hover_state_changed_;
  std::unique_ptr<views::EventMonitor> event_monitor_;
  base::OneShotTimer timer_;
  bool hovering_ = false;
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      host_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_HOVER_DETECTOR_H_
