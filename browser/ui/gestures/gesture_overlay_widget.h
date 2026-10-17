// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_OVERLAY_WIDGET_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_OVERLAY_WIDGET_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "ui/gfx/geometry/point_f.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class Widget;
}  // namespace views

namespace gestures {

class GestureTrailView;

// A click-through, semi-transparent overlay drawn above page content while a
// gesture is being recorded: a fading trail following the cursor and a label
// naming the currently-matched command. Never injected into the page itself, so
// page script can't observe or spoof gesture state.
class GestureOverlayWidget final {
 public:
  explicit GestureOverlayWidget(content::WebContents* web_contents);

  GestureOverlayWidget(const GestureOverlayWidget&) = delete;
  GestureOverlayWidget& operator=(const GestureOverlayWidget&) = delete;

  ~GestureOverlayWidget();

  // Makes the overlay widget visible, e.g. once a gesture drawing's first
  // qualifying stroke has been made.
  void Show();

  // Returns whether the overlay is currently visible.
  bool IsShown() const;

  // Hides the overlay, e.g. when a gesture drawing ends or is cancelled.
  void Hide();

  // `path` must be in the same widget-local coordinate space as
  // `blink::WebMouseEvent::PositionInWidget`, or the trail is misplaced.
  // `show_trail` and `label` are independent: either can be shown without the
  // other.
  void UpdateTrail(const std::vector<gfx::PointF>& path,
                   bool show_trail,
                   const std::u16string& label);

 private:
  const raw_ref<content::WebContents> web_contents_;  // Not owned.

  std::unique_ptr<views::Widget> widget_;

  // Not owned; this is the contents view of `widget_`, which owns it.
  raw_ptr<GestureTrailView> trail_view_ = nullptr;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_OVERLAY_WIDGET_H_
