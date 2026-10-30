// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"

#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_collection_types.h"

// Tree tab hooks. Defined in brave/browser/ui/views/tabs/tree_tab_view_utils.cc
// so that this upstream target doesn't depend on Brave targets.
namespace tree_tab_view_utils {
tabs::ConstChildrenPtrs GetChildrenForView(
    const tabs::TabCollection& collection);
bool HasTreeTabNodeChild(const tabs::TabCollection& collection);
}  // namespace tree_tab_view_utils

#include <chrome/browser/ui/views/tabs/common/tab_collection_node.cc>

// Tree tab node collections are not mirrored in the view model. Their children
// are mirrored in their place. Used by the plaster on tab_collection_node.cc.
bool TabCollectionNode::MaybeInitializeTreeTabChildren(
    const tabs::TabCollection& collection) {
  if (!tree_tab_view_utils::HasTreeTabNodeChild(collection)) {
    return false;
  }

  for (const auto& child_ptr :
       tree_tab_view_utils::GetChildrenForView(collection)) {
    AddNewChild(GetPassKey(), child_ptr, children_.size(),
                /*perform_initialization=*/true);
  }
  return true;
}

bool TabCollectionNode::MaybeDeinitializeTreeTabChildren(
    const tabs::TabCollection& collection) {
  if (!tree_tab_view_utils::HasTreeTabNodeChild(collection)) {
    return false;
  }

  // The view model doesn't have nodes for tree tab node collections, so remove
  // children based on the view model.
  while (!children_.empty()) {
    RemoveChild(GetPassKey(), children_.front()->GetHandle(),
                /*perform_deinitialization=*/true);
  }
  return true;
}
