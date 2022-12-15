/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip.h"

#include <cmath>

#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/ui_features.h"
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
#define BRAVE_CALCULATE_INSERTION_INDEX                                        \
  if (tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {      \
    if (dragged_group.has_value() && candidate_index != 0 &&                   \
        tab_strip_->controller_->IsTabPinned(candidate_index - 1))             \
      continue;                                                                \
    int distance = std::numeric_limits<int>::max();                            \
    const gfx::Rect candidate_bounds =                                         \
        candidate_index == 0                                                   \
            ? gfx::Rect()                                                      \
            : tab_strip_->tab_container_->GetIdealBounds(candidate_index - 1); \
    if (tab_strip_->controller_->IsTabPinned(first_dragged_tab_index)) {       \
      /* Pinned tabs are laid out in a grid. */                                \
      distance = std::sqrt(                                                    \
          std::pow(dragged_bounds.x() - candidate_bounds.CenterPoint().x(),    \
                   2) +                                                        \
          std::pow(dragged_bounds.y() - candidate_bounds.CenterPoint().y(),    \
                   2));                                                        \
    } else {                                                                   \
      /* Unpinned tabs are laid out vertically. So we consider only y */       \
      /* coordinate */                                                         \
      distance = std::abs(dragged_bounds.y() - candidate_bounds.bottom());     \
    }                                                                          \
    if (distance < min_distance) {                                             \
      min_distance = distance;                                                 \
      min_distance_index = candidate_index;                                    \
    }                                                                          \
    continue;                                                                  \
  }

#define BRAVE_CALCULATE_BOUNDS_FOR_DRAGGED_VIEWS                           \
  if (tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {  \
    std::vector<gfx::Rect> bounds;                                         \
    int x = 0;                                                             \
    int y = 0;                                                             \
    for (const TabSlotView* view : views) {                                \
      const int height = view->height();                                   \
      if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {     \
        if (const Tab* tab = static_cast<const Tab*>(view);                \
            tab->data().pinned) {                                          \
          /* In case it's a pinned tab, lay out them horizontally */       \
          bounds.emplace_back(x, y, tabs::kVerticalTabMinWidth, height);   \
          constexpr int kStackedOffset = 4;                                \
          x += kStackedOffset;                                             \
          continue;                                                        \
        }                                                                  \
        if (view->group().has_value()) {                                   \
          /* In case it's a tab in a group, set left padding */            \
          x = BraveTabGroupHeader::GetLeftPaddingForVerticalTabs();        \
        }                                                                  \
      }                                                                    \
      bounds.emplace_back(x, y, TabStyle::GetStandardWidth() - x, height); \
      /* unpinned dragged tabs are laid out vertically */                  \
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
