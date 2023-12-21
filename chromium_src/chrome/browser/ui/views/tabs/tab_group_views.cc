/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/tabs/tab_group_style.h"
#include "ui/views/view_utils.h"

#define TabGroupHeader BraveTabGroupHeader
#define TabGroupUnderline BraveTabGroupUnderline
#define TabGroupHighlight BraveTabGroupHighlight

// TabGroupViews destructor is not virtual, so we can't override the method.
#define BRAVE_TAB_GROUP_VIEWS_GET_LEADING_TRAILING_GROUP_VIEWS                 \
  if (tabs::utils::ShouldShowVerticalTabs(                                     \
          tab_slot_controller_->GetBrowser())) {                               \
    std::vector<raw_ptr<views::View, VectorExperimental>>                      \
        children_in_same_group;                                                \
    base::ranges::copy_if(                                                     \
        children, std::back_inserter(children_in_same_group),                  \
        [this](views::View* child) {                                           \
          TabSlotView* tab_slot_view = views::AsViewClass<TabSlotView>(child); \
          return tab_slot_view && tab_slot_view->group() == group_ &&          \
                 tab_slot_view->GetVisible();                                  \
        });                                                                    \
    if (children_in_same_group.empty()) {                                      \
      return {nullptr, nullptr};                                               \
    }                                                                          \
    return base::ranges::minmax(                                               \
        children_in_same_group,                                                \
        [](const views::View* a, const views::View* b) {                       \
          return a->GetBoundsInScreen().bottom() <                             \
                 b->GetBoundsInScreen().bottom();                              \
        });                                                                    \
  }

#include "src/chrome/browser/ui/views/tabs/tab_group_views.cc"

#undef BRAVE_TAB_GROUP_VIEWS_GET_LEADING_TRAILING_GROUP_VIEWS
#undef TabGroupHighlight
#undef TabGroupUnderline
#undef TabGroupHeader

const Browser* TabGroupViews::GetBrowser() const {
  return tab_slot_controller_->GetBrowser();
}
