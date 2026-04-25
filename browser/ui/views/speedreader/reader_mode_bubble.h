/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_BUBBLE_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_BUBBLE_H_

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
class ToggleButton;
}  // namespace views

namespace speedreader {

class SpeedreaderTabHelper;
class SpeedreaderService;

// ReaderModeBubble is shown when Speedreader is not enabled, but the user has
// chosen to distill a single page. The Speedreader service runs in one-shot
// mode, and this bubble is shown to prompt the user to enable Speedreader.
class ReaderModeBubble : public SpeedreaderBubbleView,
                         public LocationBarBubbleDelegateView {
  METADATA_HEADER(ReaderModeBubble, LocationBarBubbleDelegateView)
 public:
  ReaderModeBubble(views::View* anchor_view, SpeedreaderTabHelper* tab_helper);
  ReaderModeBubble(const ReaderModeBubble&) = delete;
  ReaderModeBubble& operator=(const ReaderModeBubble&) = delete;
  ~ReaderModeBubble() override = default;

  // SpeedreaderBubbleView:
  void Show() override;
  void Hide() override;

 private:
  // LocationBarBubbleDelegateView:
  void WindowClosing() override;
  bool ShouldShowCloseButton() const override;

  // views::BubbleDialogDelegateView:
  void Init() override;

  // views::View
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  SpeedreaderService* GetSpeedreaderService();
  void OnSiteToggled(const ui::Event& event);
  void OnAllSitesToggled(const ui::Event& event);

  raw_ptr<SpeedreaderTabHelper> tab_helper_ = nullptr;
  raw_ptr<views::ToggleButton> site_toggle_ = nullptr;
  raw_ptr<views::ToggleButton> all_sites_toggle_ = nullptr;
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_BUBBLE_H_
