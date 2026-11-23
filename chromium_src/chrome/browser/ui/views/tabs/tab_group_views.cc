/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <iterator>
#include <optional>
#include <tuple>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_group_style_views.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "chrome/browser/ui/views/tabs/tab_slot_view.h"
#include "components/tab_groups/tab_group_id.h"
#include "ui/views/view_utils.h"

namespace {

// Returns the leading and trailing views of the group when vertical tabs are
// shown, or std::nullopt when upstream's horizontal logic should be used.
std::optional<std::tuple<views::View*, views::View*>>
GetBraveVerticalTabsLeadingTrailingGroupViews(
    TabSlotController& tab_slot_controller,
    const tab_groups::TabGroupId& group,
    const std::vector<raw_ptr<views::View, VectorExperimental>>& children) {
  auto* vertical_tab_controller = VerticalTabController::From(
      tab_slot_controller.GetBrowserWindowInterface());
  if (!vertical_tab_controller ||
      !vertical_tab_controller->ShouldShowBraveVerticalTabs()) {
    return std::nullopt;
  }

  std::vector<raw_ptr<views::View, VectorExperimental>> children_in_same_group;
  std::ranges::copy_if(
      children, std::back_inserter(children_in_same_group),
      [&group](views::View* child) {
        TabSlotView* tab_slot_view = views::AsViewClass<TabSlotView>(child);
        return tab_slot_view && tab_slot_view->group() == group &&
               tab_slot_view->GetVisible();
      });
  if (children_in_same_group.empty()) {
    return std::tuple<views::View*, views::View*>{nullptr, nullptr};
  }
  auto [min, max] = std::ranges::minmax(
      children_in_same_group, [](const views::View* a, const views::View* b) {
        return a->GetBoundsInScreen().bottom() <
               b->GetBoundsInScreen().bottom();
      });
  return std::tuple<views::View*, views::View*>{min, max};
}

}  // namespace

#include <chrome/browser/ui/views/tabs/tab_group_views.cc>

const BrowserWindowInterface* TabGroupViews::GetBrowserWindowInterface() const {
  return tab_slot_controller_->GetBrowserWindowInterface();
}
