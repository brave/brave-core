/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_HOVER_CARD_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_HOVER_CARD_BUBBLE_VIEW_H_

// Inject a protected method that will have access to the private members of the
// base class. Then, we can call this method from the subclass' override.
#include "ui/gfx/image/image_skia.h"

class TabHoverCardBubbleView;

#define BRAVE_TAB_HOVER_CARD_BUBBLE_VIEW_H_ \
  bool has_thumbnail_view() {               \
    return thumbnail_view_;                 \
  }                                         \
                                            \
 protected:                                 \
  void BraveUpdateCardContent(const Tab* tab, bool discarded);

#define TabHoverCardBubbleView TabHoverCardBubbleView_ChromiumImpl
#define UpdateCardContent virtual UpdateCardContent
#define SetTargetTabImage virtual SetTargetTabImage
#define SetPlaceholderImage virtual SetPlaceholderImage
#include "src/chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"  // IWYU pragma: export
#undef SetPlaceholderImage
#undef SetTargetTabImage
#undef UpdateCardContent
#undef TabHoverCardBubbleView
#undef BRAVE_TAB_HOVER_CARD_BUBBLE_VIEW_H_

class TabHoverCardBubbleView : public TabHoverCardBubbleView_ChromiumImpl {
 public:
  using TabHoverCardBubbleView_ChromiumImpl::
      TabHoverCardBubbleView_ChromiumImpl;

  TabHoverCardBubbleView(const TabHoverCardBubbleView&) = delete;
  TabHoverCardBubbleView& operator=(const TabHoverCardBubbleView&) = delete;

  void UpdateCardContent(const Tab* tab, bool discarded) override;
  void SetTargetTabImage(gfx::ImageSkia preview_image) override;
  void SetPlaceholderImage() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_HOVER_CARD_BUBBLE_VIEW_H_
