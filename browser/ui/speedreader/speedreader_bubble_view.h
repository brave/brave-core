/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_VIEW_H_

#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "ui/views/controls/styled_label.h"

namespace views {
class View;
}  // namespace views

namespace speedreader {

// Interface to display a Speedreader info bubble.
// This object is responsible for its own lifetime.
class SpeedreaderBubbleView : public LocationBarBubbleDelegateView {
 public:
  explicit SpeedreaderBubbleView(views::View* anchor_view)
      : LocationBarBubbleDelegateView(anchor_view, nullptr) {}

  ~SpeedreaderBubbleView() override = default;

  // Shows the bubble
  virtual void Show() = 0;

  // Closes the bubble and prevents future calls into the controller
  virtual void Hide() = 0;
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_SPEEDREADER_SPEEDREADER_BUBBLE_VIEW_H_
