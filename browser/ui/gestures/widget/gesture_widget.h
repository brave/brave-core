// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_WIDGET_H_
#define BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_WIDGET_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class Widget;
}  // namespace views

namespace gestures {

class GestureView;

// A click-through, semi-transparent overlay drawn above page content while a
// gesture is being recorded, a fading trail following the cursor and a label
// naming the currently-matched command. Never injected into the page itself,
// so page script can't observe or spoof gesture state.
class GestureWidget final {
 public:
  explicit GestureWidget(content::WebContents* web_contents);

  GestureWidget(const GestureWidget&) = delete;
  GestureWidget& operator=(const GestureWidget&) = delete;

  ~GestureWidget();

  // Makes the overlay widget visible, e.g. once a gesture drawing's first
  // qualifying stroke has been made.
  void Show();

  // Returns whether the overlay is currently visible.
  bool IsVisible() const;

  // Hides the overlay, e.g. when a gesture drawing ends or is cancelled.
  void Hide();

  // `path` must be in the same widget-local coordinate space as
  // `blink::WebMouseEvent::PositionInWidget`, or the trail is misplaced.
  // `show_trail` and `label` are independent, either can be shown without
  // the other.
  void UpdateTrail(bool show_trail,
                   const GesturePath& path,
                   const std::u16string& label);

 private:
  const raw_ref<content::WebContents> web_contents_;

  std::unique_ptr<views::Widget> widget_;

  // This is the contents view of `widget_`, which owns it.
  raw_ptr<GestureView> gesture_view_ = nullptr;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_WIDGET_H_
