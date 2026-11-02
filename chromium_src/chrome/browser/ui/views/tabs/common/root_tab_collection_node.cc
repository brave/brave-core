// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <chrome/browser/ui/views/tabs/common/root_tab_collection_node.cc>

// Tree tab nodes aren't mirrored as views, so tab views are direct children of
// the unpinned container, and it's that container's layout (position, indent,
// visibility) that depends on the tree. Invalidating this node's own view would
// only invalidate its ancestors, so invalidate the unpinned container.
void RootTabCollectionNode::OnTreeTabChanged(const TreeTabChange& change) {
  for (const auto& child : children()) {
    if (child->type() == Type::UNPINNED && child->view()) {
      child->view()->InvalidateLayout();
    }
  }
}
