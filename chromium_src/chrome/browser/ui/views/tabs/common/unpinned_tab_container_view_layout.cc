// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/unpinned_tab_container_view_layout.h"

#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"

// Indent tree tabs by their level. Only when the tab strip is expanded, as
// there's no room for indentation when collapsed.
#define BRAVE_UNPINNED_TAB_CONTAINER_VIEW_LAYOUT_CALCULATE_VERTICAL_LAYOUT \
  if (collapse_state == tabs::VerticalTabStripCollapseState::kExpanded &&  \
      size_bounds.width().is_bounded()) {                                  \
    x += tree_tab_view_utils::GetTreeTabIndent(                            \
        child, size_bounds.width().value() - x - horizontal_padding);      \
  }

// Hide tree tabs whose ancestor is collapsed.
#define BRAVE_UNPINNED_TAB_CONTAINER_VIEW_LAYOUT_IS_CHILD_VISIBLE_IN_CONTAINER \
  if (tree_tab_view_utils::IsInCollapsedTreeTabNode(child)) {                  \
    return false;                                                              \
  }

#include <chrome/browser/ui/views/tabs/common/unpinned_tab_container_view_layout.cc>

#undef BRAVE_UNPINNED_TAB_CONTAINER_VIEW_LAYOUT_IS_CHILD_VISIBLE_IN_CONTAINER
#undef BRAVE_UNPINNED_TAB_CONTAINER_VIEW_LAYOUT_CALCULATE_VERTICAL_LAYOUT
