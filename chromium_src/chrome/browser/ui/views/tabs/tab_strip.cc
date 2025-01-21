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
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
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

// Macros for TabDragContextImpl. TabDragContextImpl is in anonymous namespace
// and it's used in the tab_strip.cc file at the same time. Because of this,
// the class is really hard to be extended with inheritance, so using patch file
// seems to be the most efficient way for now. If we could split this into
// another file or child class, that'd be great.
#define BRAVE_TAB_DRAG_CONTEXT_IMPL_CALCULATE_INSERTION_INDEX                \
  if (tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {       \
    tabs::UpdateInsertionIndexForVerticalTabs(                               \
        dragged_bounds, first_dragged_tab_index, num_dragged_tabs,           \
        dragged_group, candidate_index, tab_strip_->controller_.get(),       \
        &tab_strip_->tab_container_.get(), min_distance, min_distance_index, \
        tab_strip_);                                                         \
    continue;                                                                \
  }

#define BRAVE_TAB_DRAG_CONTEXT_IMPL_CALCULATE_BOUNDS_FOR_DRAGGED_VIEWS      \
  if (tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {      \
    return tabs::CalculateBoundsForVerticalDraggedViews(views, tab_strip_); \
  }

#include "src/chrome/browser/ui/views/tabs/tab_strip.cc"

#undef BRAVE_TAB_DRAG_CONTEXT_IMPL_CALCULATE_BOUNDS_FOR_DRAGGED_VIEWS
#undef BRAVE_TAB_DRAG_CONTEXT_IMPL_CALCULATE_INSERTION_INDEX
#undef TabHoverCardController
#undef CompoundTabContainer
#undef TabContainerImpl
#undef AddTab

bool TabStrip::IsTabTiled(const Tab* tab) const {
  return false;
}

bool TabStrip::IsFirstTabInTile(const Tab* tab) const {
  return false;
}
