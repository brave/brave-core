// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/tab_strip_collection_controller.h"

class TabStripModel;
struct TabStripUserGestureDetails;

// Tree tab hook. Defined in brave/browser/ui/views/tabs/tree_tab_view_utils.cc
// so that this upstream target doesn't depend on Brave targets.
namespace tree_tab_view_utils {
void ActivateTabAndSelectTreeTabSubtree(
    TabStripModel* tab_strip_model,
    int index,
    const TabStripUserGestureDetails& detail);
}  // namespace tree_tab_view_utils

#include <chrome/browser/ui/views/tabs/common/tab_strip_collection_controller.cc>

void TabStripCollectionController::UpdateTreeToggleButton(
    const tabs::TabInterface* tab) {
  TabCollectionNode* node = root_node_->GetNodeForHandle(tab->GetHandle());
  if (auto* tab_view =
          views::AsViewClass<TabView>(node ? node->view() : nullptr)) {
    tab_view->UpdateTreeToggleButton();
  }
}
