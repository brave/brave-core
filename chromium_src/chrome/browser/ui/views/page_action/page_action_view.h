// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_

#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"

// Add methods to override the IconLabelBubbleView methods.
// Also add a friend class for testing.
#define ShouldShowLabelAfterAnimation()                        \
  ShouldShowLabelAfterAnimation() const override;              \
  FRIEND_TEST_ALL_PREFIXES(PageActionViewTest,                 \
                           AlwaysShowsLabelEnsuresLabelWidth); \
  bool ShouldShowLabel() const override;                       \
  SkColor GetBackgroundColor() const override;                 \
  SkColor GetForegroundColor() const override;                 \
  bool ShouldAlwaysShowLabel()

#include <chrome/browser/ui/views/page_action/page_action_view.h>  // IWYU pragma: export

#undef ShouldShowLabelAfterAnimation

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_
