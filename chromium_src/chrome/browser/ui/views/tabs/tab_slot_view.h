// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef TAB_SLOT_VIEW_H_
#define TAB_SLOT_VIEW_H_

#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "ui/views/view.h"

// Add a method to TreeTabNodeId to TabSlotView.
#define SetGroup(...)                                                        \
  SetGroup_Unused() const {}                                                 \
  void set_tree_tab_node(const std::optional<tree_tab::TreeTabNodeId>& id) { \
    tree_tab_node_id_ = id;                                                  \
  }                                                                          \
  const std::optional<tree_tab::TreeTabNodeId>& tree_tab_node() const {      \
    return tree_tab_node_id_;                                                \
  }                                                                          \
                                                                             \
 private:                                                                    \
  std::optional<tree_tab::TreeTabNodeId> tree_tab_node_id_;                  \
                                                                             \
 public:                                                                     \
  virtual void SetGroup(__VA_ARGS__)

// Add a method to get TabNestingInfo in addition to TabSizeINfo.
#define GetTabSizeInfo(...)              \
  GetTabSizeInfo(__VA_ARGS__) const = 0; \
  virtual TabNestingInfo GetTabNestingInfo()

#include <chrome/browser/ui/views/tabs/tab_slot_view.h>

#undef GetTabSizeInfo
#undef SetGroup

#endif  // TAB_SLOT_VIEW_H_
