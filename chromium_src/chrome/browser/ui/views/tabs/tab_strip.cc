/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_container_impl.h"

#if BUILDFLAG(IS_WIN)
#include "ui/gfx/win/hwnd_util.h"
#endif

#define AddTab(TAB, MODEL_INDEX, PINNED) \
  AddTab(std::make_unique<BraveTab>(this), MODEL_INDEX, PINNED)
#define TabHoverCardController BraveTabHoverCardController
#define TabContainer BraveTabContainer
#define TAB_DRAG_CONTEXT_CALCULATE_INSERTION_INDEX_FOR_VERTICAL_TABS           \
  if (tabs::features::ShouldShowVerticalTabs()) {                              \
    const int ideal_y =                                                        \
        candidate_index == 0 ? 0 : ideal_bounds(candidate_index - 1).bottom(); \
    const int distance = std::abs(dragged_bounds.y() - ideal_y);               \
    if (distance < min_distance) {                                             \
      min_distance = distance;                                                 \
      min_distance_index = candidate_index;                                    \
    }                                                                          \
    continue;                                                                  \
  }

#define TAB_DRAG_CONTEXT_CALCULATE_BOUNDS_FOR_DRAGGED_VIEWS_VERTICAL_TABS \
  if (tabs::features::ShouldShowVerticalTabs()) {                         \
    std::vector<gfx::Rect> bounds;                                        \
    int y = 0;                                                            \
    for (const TabSlotView* view : views) {                               \
      const int height = view->height();                                  \
      bounds.push_back(gfx::Rect(0, y, view->width(), view->height()));   \
      y += height;                                                        \
    }                                                                     \
    return bounds;                                                        \
  }

#include "src/chrome/browser/ui/views/tabs/tab_strip.cc"

#undef BRAVE_TAB_STRIP_CALCULATE_DRAGGED_VIEW_BOUNDS_FOR_VERTICAL_TABS
#undef BRAVE_TAB_STRIP_CALCULATE_INDEX_FOR_VERTICAL_TABS
#undef TabContainer
#undef TabHoverCardController
#undef AddTab
