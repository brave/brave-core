/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_HOVER_DETECTOR_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_HOVER_DETECTOR_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class EventMonitor;
}  // namespace views

// Detects when the mouse is "hovering" in a consumer-defined region of a host
// widget's window (typically one of the edges of the browser window) and emits
// a debounced signal suitable for driving auto-hide UI reveal.
class EdgeHoverDetector : public views::WidgetObserver,
                          public ui::EventObserver {
 public:
  using IsHoverPointCallback =
      base::RepeatingCallback<bool(const gfx::Point& screen_point)>;

  using HoverStateChangedCallback =
      base::RepeatingCallback<void(bool hovering)>;

  // Debounce delays applied before committing a hover state change.
  static constexpr base::TimeDelta kMouseEnterDelay = base::Milliseconds(60);
  static constexpr base::TimeDelta kMouseExitDelay = base::Milliseconds(500);

  // Constructs a new detector for the specified host widget window. The caller
  // provides the following callbacks:
  //   - `is_hover_point`: Returns true if the specified screen coordinate is
  //     considered within the edge's active region.
  //   - `hover_state_changed`: Called when the hover state has changed.
  EdgeHoverDetector(views::Widget* host,
                    IsHoverPointCallback is_hover_point,
                    HoverStateChangedCallback hover_state_changed);

  EdgeHoverDetector(const EdgeHoverDetector&) = delete;
  EdgeHoverDetector& operator=(const EdgeHoverDetector&) = delete;
  ~EdgeHoverDetector() override;

  // Current committed hover state (post-debounce).
  bool hovering() const { return hovering_; }

  // Forces a re-evaluation of hover state using the current screen cursor
  // position. This can be used to manually force hover detection when the
  // "edge" geometry has changed without mouse input.
  void DetectHoverState();

  // Dispatches an event for testing, bypassing `views::EventMonitor`.
  void HandleEventForTesting(const ui::Event& event);

 private:
  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  // ui::EventObserver:
  void OnEvent(const ui::Event& event) override;

  bool IsCursorHovering();
  void ApplyWithDelay(bool hovering);
  void ToggleHoverState();

  IsHoverPointCallback is_hover_point_;
  HoverStateChangedCallback hover_state_changed_;
  std::unique_ptr<views::EventMonitor> event_monitor_;
  base::OneShotTimer timer_;
  bool hovering_ = false;
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      host_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_HOVER_DETECTOR_H_
