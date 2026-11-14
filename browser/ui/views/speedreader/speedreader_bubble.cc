/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/speedreader/reader_mode_bubble.h"
#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace speedreader {

SpeedreaderBubbleView* ShowSpeedreaderBubble(
    BrowserWindowInterface& browser,
    SpeedreaderTabHelper& tab_helper,
    SpeedreaderBubbleLocation location) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(&browser);
  if (!browser_view) {
    return nullptr;
  }

  views::View* anchor = nullptr;
  views::BubbleBorder::Arrow arrow = views::BubbleBorder::NONE;
  switch (location) {
    case SpeedreaderBubbleLocation::kLocationBar:
      anchor = browser_view->GetLocationBarView();
      arrow = views::BubbleBorder::TOP_RIGHT;
      break;
    case SpeedreaderBubbleLocation::kToolbar:
      anchor = BraveContentsContainerView::From(
                   browser_view->GetActiveContentsContainerView())
                   ->reader_mode_toolbar()
                   ->toolbar();
      arrow = views::BubbleBorder::TOP_LEFT;
      break;
  }

  auto* reader_mode_bubble = new ReaderModeBubble(anchor, &tab_helper);
  views::BubbleDialogDelegateView::CreateBubble(reader_mode_bubble);
  reader_mode_bubble->SetArrow(arrow);
  reader_mode_bubble->Show();
  return reader_mode_bubble;
}

}  // namespace speedreader
