// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMMON_ROOT_TAB_COLLECTION_NODE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMMON_ROOT_TAB_COLLECTION_NODE_H_

#include <vector>

#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "components/tabs/public/tab_collection_observer.h"

// When tree tabs are on, Brave's tree tab collection delegate restructures the
// collection hierarchy without sending TabCollectionObserver notifications,
// and tree tab node collections are not mirrored in the view model (see
// tree_tab_view_utils.h). So instead of applying each notification, the view
// model is synced with the collection hierarchy.
#define OnChildMoved                                                           \
  OnChildMoved_ChromiumImpl(const tabs::TabCollection::Position& to_position,  \
                            const NodeData& node_data);                        \
  void OnChildrenAdded_ChromiumImpl(                                           \
      const tabs::TabCollection::Position& position,                           \
      const tabs::TabCollectionNodes& handles, bool insert_from_detached);     \
  void OnChildrenRemoved_ChromiumImpl(                                         \
      const tabs::TabCollection::Position& position,                           \
      const tabs::TabCollectionNodes& handles);                                \
  void OnTabStripModelChanged_ChromiumImpl(                                    \
      TabStripModel* tab_strip_model, const TabStripModelChange& change,       \
      const TabStripSelectionChange& selection);                               \
  void OnTreeTabChanged(const TreeTabChange& change) override;                 \
                                                                               \
  struct TreeTabsSyncResult {                                                  \
    tabs::TabCollectionNodes added;                                            \
    std::vector<tabs::TabCollectionNodeHandle> moved;                          \
    bool removed = false;                                                      \
  };                                                                           \
  /* Returns true if the view model should be synced with the collection    */ \
  /* hierarchy instead of applying a notification about |parent_handles|.   */ \
  bool ShouldSyncWithTreeTabs(                                                 \
      const std::vector<tabs::TabCollection::Handle>& parent_handles);         \
  void SyncWithTreeTabs();                                                     \
  void ScheduleSyncWithTreeTabs();                                             \
  void SyncChildrenWithTreeTabs(TabCollectionNode* node,                       \
                                TreeTabsSyncResult& result);                   \
  void RemoveStaleChildren(TabCollectionNode* node,                            \
                           TreeTabsSyncResult& result);                        \
  bool IsInPinnedContainer(const TabCollectionNode* node);                     \
  /* Expands collapsed tree tab ancestors of |tab| so that it's visible.    */ \
  void ExpandCollapsedTreeTabAncestors(const tabs::TabInterface* tab);         \
  bool tree_tabs_sync_pending_ = false;                                        \
  void OnChildMoved

#include <chrome/browser/ui/views/tabs/common/root_tab_collection_node.h>  // IWYU pragma: export

#undef OnChildMoved

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMMON_ROOT_TAB_COLLECTION_NODE_H_
