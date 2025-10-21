// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_

#include <map>

#include "brave/components/tabs/public/tree_tab_node.h"

// A model that contains TreeTabNodes. TreeTabNode provides metadata for tabs
// related to tree tab functionality, such as level, height, and collapsed
// state. A UI components except TabStripModel should only access TreeTabNodes
// via this model and should not access TreeTabNodeTabCollection directly.
class TreeTabModel {
 public:
  TreeTabModel();
  ~TreeTabModel();

  const tabs::TreeTabNode* GetNode(const tree_tab::TreeTabNodeId& id) const;
  int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const;

  void AddTreeTabNode(tabs::TreeTabNode& node);
  void RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id);

 private:
  std::map<tree_tab::TreeTabNodeId, raw_ptr<tabs::TreeTabNode>> tree_tab_nodes_;
};

#endif  // BRAVE_BROWSER_UI_TABS_TREE_TAB_MODEL_H_
