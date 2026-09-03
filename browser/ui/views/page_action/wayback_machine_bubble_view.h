/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_BUBBLE_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_anchor.h"

namespace actions {
class ActionItem;
}  // namespace actions

namespace content {
class WebContents;
}  // namespace content

// Offers to look up the current page in the Internet Archive after a failed
// navigation. Derives from LocationBarBubbleDelegateView so that it can be
// shown either in response to a user gesture or automatically, and so that it
// closes when the tab is hidden or the browser enters fullscreen.
class WaybackMachineBubbleView : public LocationBarBubbleDelegateView {
  METADATA_HEADER(WaybackMachineBubbleView, LocationBarBubbleDelegateView)

 public:
  WaybackMachineBubbleView(views::BubbleAnchor anchor,
                           content::WebContents* web_contents,
                           actions::ActionItem* item);
  ~WaybackMachineBubbleView() override;

 private:
  void OnAccepted();
  void OnDontAskAgain();

  raw_ptr<actions::ActionItem> item_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_BUBBLE_VIEW_H_
