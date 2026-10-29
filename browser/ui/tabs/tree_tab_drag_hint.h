// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_TREE_TAB_DRAG_HINT_H_
#define BRAVE_BROWSER_UI_TABS_TREE_TAB_DRAG_HINT_H_

#include "components/tabs/public/tab_interface.h"

// Describes how dragged tabs should be nested relative to |target| when they
// are moved in tree tab mode.
enum class TreeTabDragHint {
  // Become a sibling of |target|. Placed right after |target| when it is the
  // tab above the drop position, or right before it when it is the tab below.
  kSibling,
  // Become the first child of |target|, which must be the tab above the drop
  // position.
  kChild,
};

// |target| must be the tab right above or right below the drop position. An
// empty |target| with kSibling means the drop position is the end of the strip
// and the moved tabs become top-level nodes. When the hint can't be honored at
// the move's destination, the default nesting is used.
struct TreeTabDragTarget {
  TreeTabDragHint hint = TreeTabDragHint::kSibling;
  tabs::TabHandle target;

  friend bool operator==(const TreeTabDragTarget&,
                         const TreeTabDragTarget&) = default;
};

#endif  // BRAVE_BROWSER_UI_TABS_TREE_TAB_DRAG_HINT_H_
