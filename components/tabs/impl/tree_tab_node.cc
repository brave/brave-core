// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node.h"

#include "base/logging.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"

namespace tabs {

TreeTabNode::TreeTabNode(TreeTabNodeTabCollection& collection,
                         const tree_tab::TreeTabNodeId& id)
    : collection_(collection), id_(id) {}

int TreeTabNode::GetTreeHeight() const {
  return collection_->GetTopLevelAncestor()->node().height();
}

const TabInterface* TreeTabNode::GetTab() const {
  return collection_->current_tab() ? collection_->current_tab().get()
                                    : nullptr;
}

}  // namespace tabs
