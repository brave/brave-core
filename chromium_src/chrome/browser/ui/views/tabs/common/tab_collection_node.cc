// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"

#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"

#define BRAVE_GET_TYPE_FROM_NODE             \
  case tabs::TabCollection::Type::TREE_NODE: \
    return TabCollectionNode::Type::TREE_NODE;

// Tree tab node collections are not mirrored in the view model. See
// tree_tab_view_utils.h.
#define BRAVE_TAB_COLLECTION_NODE_CREATE_VIEW_FOR_NODE \
  case Type::TREE_NODE:                                \
    NOTREACHED();

// Mirror the children of tree tab node collections in place of the tree tab
// node collections.
#define BRAVE_TAB_COLLECTION_NODE_INITIALIZE                     \
  if (tree_tab_view_utils::HasTreeTabNodeChild(*collection)) {   \
    for (const auto& child_ptr :                                 \
         tree_tab_view_utils::GetChildrenForView(*collection)) { \
      AddNewChild(GetPassKey(), child_ptr, children_.size(),     \
                  /*perform_initialization=*/true);              \
    }                                                            \
    return node_view;                                            \
  }

// The view model doesn't have nodes for tree tab node collections, so remove
// children based on the view model.
#define BRAVE_TAB_COLLECTION_NODE_DEINITIALIZE                  \
  if (tree_tab_view_utils::HasTreeTabNodeChild(*collection)) {  \
    while (!children_.empty()) {                                \
      RemoveChild(GetPassKey(), children_.front()->GetHandle(), \
                  /*perform_deinitialization=*/true);           \
    }                                                           \
    return;                                                     \
  }

#include <chrome/browser/ui/views/tabs/common/tab_collection_node.cc>

#undef BRAVE_TAB_COLLECTION_NODE_DEINITIALIZE
#undef BRAVE_TAB_COLLECTION_NODE_INITIALIZE
#undef BRAVE_TAB_COLLECTION_NODE_CREATE_VIEW_FOR_NODE
#undef BRAVE_GET_TYPE_FROM_NODE
