/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_MODE_BUBBLE_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_MODE_BUBBLE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace content {
class WebContents;
}  // namespace content

namespace ui {
class Event;
}  // namespace ui

namespace views {
class View;
class Label;
class ToggleButton;
class StyledLabel;
}  // namespace views

namespace speedreader {

class SpeedreaderTabHelper;

// SpeedreaderModeBubble is shown when Speedreader is enabled, a page has been
// automatically distilled, and the user clicked the reader icon in the omnibox.
// The bubble shows the website name and allows the user to disable future
// automatic distillations for just this domain.
class SpeedreaderModeBubble : public SpeedreaderBubbleView,
                              public LocationBarBubbleDelegateView {
 public:
  METADATA_HEADER(SpeedreaderModeBubble);
  SpeedreaderModeBubble(views::View* anchor_view,
                        SpeedreaderTabHelper* tab_helper);
  SpeedreaderModeBubble(const SpeedreaderModeBubble&) = delete;
  SpeedreaderModeBubble& operator=(const SpeedreaderModeBubble&) = delete;
  ~SpeedreaderModeBubble() override = default;

  // SpeedreaderBubbleView:
  void Show() override;
  void Hide() override;

 private:
  // LocationBarBubbleDelegateView:
  void WindowClosing() override;
  bool ShouldShowCloseButton() const override;

  // views::BubbleDialogDelegateView:
  void Init() override;
  void OnThemeChanged() override;

  // views::View:
  void AddedToWidget() override;
  gfx::Size CalculatePreferredSize() const override;

  void UpdateColors();
  void OnButtonPressed(const ui::Event& event);
  void OnLinkClicked(const ui::Event& event);

  raw_ptr<SpeedreaderTabHelper> tab_helper_ = nullptr;  // weak.

  views::StyledLabel* site_title_label_ = nullptr;         // weak.
  views::ToggleButton* site_toggle_button_ = nullptr;      // weak.
  views::StyledLabel* site_toggle_explanation_ = nullptr;  // weak.
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_MODE_BUBBLE_H_
