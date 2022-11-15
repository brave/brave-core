/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip.h"

#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/compound_tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_container_impl.h"

#if BUILDFLAG(IS_WIN)
#include "ui/gfx/win/hwnd_util.h"
#endif

#define AddTab(TAB, MODEL_INDEX, PINNED) \
  AddTab(std::make_unique<BraveTab>(this), MODEL_INDEX, PINNED)
#define CompoundTabContainer BraveCompoundTabContainer
#define TabContainerImpl BraveTabContainer
#define TabHoverCardController BraveTabHoverCardController
#define BRAVE_CALCULATE_INSERTION_INDEX                                       \
  if (tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {     \
    const int ideal_y =                                                       \
        candidate_index == 0                                                  \
            ? 0                                                               \
            : tab_strip_->tab_container_->GetIdealBounds(candidate_index - 1) \
                  .bottom();                                                  \
    const int distance = std::abs(dragged_bounds.y() - ideal_y);              \
    if (distance < min_distance) {                                            \
      min_distance = distance;                                                \
      min_distance_index = candidate_index;                                   \
    }                                                                         \
    continue;                                                                 \
  }

#define BRAVE_CALCULATE_BOUNDS_FOR_DRAGGED_VIEWS                           \
  if (tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {  \
    std::vector<gfx::Rect> bounds;                                         \
    int y = 0;                                                             \
    for (const TabSlotView* view : views) {                                \
      int x = 0;                                                           \
      if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab &&     \
          view->group().has_value()) {                                     \
        x = BraveTabGroupHeader::GetLeftPaddingForVerticalTabs();          \
      }                                                                    \
      const int height = view->height();                                   \
      bounds.emplace_back(x, y, TabStyle::GetStandardWidth() - x, height); \
      y += height;                                                         \
    }                                                                      \
    return bounds;                                                         \
  }

#include "src/chrome/browser/ui/views/tabs/tab_strip.cc"

#undef BRAVE_CALCULATE_BOUNDS_FOR_DRAGGED_VIEWS
#undef BRAVE_CALCULATE_INSERTION_INDEX
#undef TabHoverCardController
#undef CompoundTabContainer
#undef TabContainerImpl
#undef AddTab
