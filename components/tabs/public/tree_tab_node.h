// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_
#define BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_

#include <memory>

#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"

// TreeTabNode is a specialized TabCollection that represents a node in the
// tree structure of tabs. It contains a current tab and can have child
// collections, such as other TreeTabNodes, TabGroupTabCollections,
// SplitTabCollections.
class TreeTabNode : public tabs::TabCollection {
 public:
  TreeTabNode(const tree_tab::TreeTabNodeId& tree_tab_node_id,
              std::unique_ptr<tabs::TabInterface> current_tab);
  ~TreeTabNode() override;

  const tree_tab::TreeTabNodeId& tree_tab_node_id() const {
    return tree_tab_node_id_;
  }

  const tabs::TabInterface& current_tab() const { return current_tab_.get(); }
  tabs::TabInterface& current_tab() { return current_tab_.get(); }

 private:
  tree_tab::TreeTabNodeId tree_tab_node_id_;
  raw_ref<tabs::TabInterface> current_tab_;
};

#endif  // BRAVE_COMPONENTS_TABS_PUBLIC_TREE_TAB_NODE_H_
