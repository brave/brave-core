// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/vertical/tab_collection_node.h"

#define BRAVE_GET_TYPE_FROM_NODE             \
  case tabs::TabCollection::Type::TREE_NODE: \
    return TabCollectionNode::Type::TREE_NODE;

#define BRAVE_TAB_COLLECTION_NODE_CREATE_VIEW_FOR_NODE \
  case Type::TREE_NODE:                                \
    NOTREACHED();

#include <chrome/browser/ui/views/tabs/vertical/tab_collection_node.cc>
#undef BRAVE_TAB_COLLECTION_NODE_CREATE_VIEW_FOR_NODE
#undef BRAVE_GET_TYPE_FROM_NODE
