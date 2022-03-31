/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_HOVER_CARD_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_HOVER_CARD_BUBBLE_VIEW_H_

// Inject a protected method that will have access to the private members of the
// base class. Then, we can call this method from the subclass' override.
#define BRAVE_TAB_HOVER_CARD_BUBBLE_VIEW_H_ \
 protected:                                 \
  void BraveUpdateCardContent(const Tab* tab);

#define TabHoverCardBubbleView TabHoverCardBubbleView_ChromiumImpl
#define UpdateCardContent virtual UpdateCardContent
#include "src/chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"
#undef UpdateCardContent
#undef TabHoverCardBubbleView
#undef BRAVE_TAB_HOVER_CARD_BUBBLE_VIEW_H_

class TabHoverCardBubbleView : public TabHoverCardBubbleView_ChromiumImpl {
 public:
  using TabHoverCardBubbleView_ChromiumImpl::
      TabHoverCardBubbleView_ChromiumImpl;

  TabHoverCardBubbleView(const TabHoverCardBubbleView&) = delete;
  TabHoverCardBubbleView& operator=(const TabHoverCardBubbleView&) = delete;

  void UpdateCardContent(const Tab* tab) override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_HOVER_CARD_BUBBLE_VIEW_H_
