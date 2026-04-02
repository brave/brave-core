// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_

#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"
#include "chrome/browser/ui/views/page_action/page_action_model_observer.h"
#include "ui/views/view.h"
#include "ui/views/widget/native_widget_delegate.h"

// Add methods to override the IconLabelBubbleView methods.
// Also add a friend class for testing.
#define ShouldShowLabelAfterAnimation()                                 \
  ShouldShowLabelAfterAnimation() const override;                       \
  FRIEND_TEST_ALL_PREFIXES(PageActionViewTest,                          \
                           AlwaysShowsLabelEnsuresLabelWidth);          \
  views::ProposedLayout CalculateProposedLayout(                        \
      const views::SizeBounds& size_bounds) const override;             \
  gfx::Size GetSizeForLabelWidth(int label_width) const override;       \
  bool ShouldShowLabel() const override;                                \
  SkColor GetBackgroundColor() const override;                          \
  SkColor GetForegroundColor() const override;                          \
  std::optional<int> GetOverrideHeight() const;                         \
  void OnPageActionModelVisualRefresh(PageActionModelInterface* model); \
  bool ShouldAlwaysShowLabel()

// Make a OnPageActionModelChanged wrapper
#define OnPageActionModelChanged(...)             \
  OnPageActionModelChanged_Chromium(__VA_ARGS__); \
  void OnPageActionModelChanged(__VA_ARGS__)

// Make a GetMinimumSize wrapper
#define GetMinimumSize()           \
  GetMinimumSize_Chromium() const; \
  gfx::Size GetMinimumSize()

// Make a OnNewActiveController wrapper
#define OnNewActiveController(...)             \
  OnNewActiveController_Chromium(__VA_ARGS__); \
  void OnNewActiveController(__VA_ARGS__)

#include <chrome/browser/ui/views/page_action/page_action_view.h>  // IWYU pragma: export

#undef OnNewActiveController
#undef GetMinimumSize
#undef OnPageActionModelChanged
#undef ShouldShowLabelAfterAnimation

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_
