// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/tree_tab_node.h"

#include <memory>
#include <utility>

TreeTabNode::TreeTabNode(const tree_tab::TreeTabNodeId& tree_tab_node_id,
                         std::unique_ptr<tabs::TabInterface> current_tab)
    : TabCollection(TabCollection::Type::TREE_NODE,
                    /*supported_child_collections=*/
                    {TabCollection::Type::SPLIT, TabCollection::Type::GROUP,
                     TabCollection::Type::TREE_NODE},
                    /*supports_tabs=*/true),
      tree_tab_node_id_(tree_tab_node_id),
      current_tab_(*current_tab) {
  CHECK(!tree_tab_node_id.is_empty());
  CHECK(current_tab);

  AddTab(std::move(current_tab), 0);
}

TreeTabNode::~TreeTabNode() {}
