/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_strip_model.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "components/tabs/public/tab_strip_collection.h"

namespace sidebar {

// A sidebar web panel is backed by a pinned tab, but its contents stays
// attached to the panel's own ContentsWebView for as long as the panel is
// open. Putting that tab in a split makes BrowserView::ShowSplitView() attach
// the same contents to a second ContentsWebView, which breaks every
// one-view-per-contents invariant at once - the TabStripModel re-entrancy
// guard via the resulting focus notification, ReadAnything's
// one-observer-per-tab CHECK, and so on. Refused in the model so no entry
// point can do it, including the ones with no Brave-side UI hook (bookmarks
// and links "open in split view", the contents drop target, extensions).
bool IsWebPanelTab(tabs::TabInterface* tab);

}  // namespace sidebar

namespace {

bool BraveIsWebPanelTabAt(const TabStripModel* model, int index) {
  return model->ContainsIndex(index) &&
         sidebar::IsWebPanelTab(model->GetTabAtIndex(index));
}

bool BraveNewSplitWouldContainWebPanel(const TabStripModel* model,
                                       const std::vector<int>& indices) {
  // With a single index AddToNewSplit() uses the active tab as the pivot, so
  // the active tab ends up in the split as well.
  if (indices.size() == 1u && sidebar::IsWebPanelTab(model->GetActiveTab())) {
    return true;
  }

  return std::ranges::any_of(indices, [model](int index) {
    return BraveIsWebPanelTabAt(model, index);
  });
}

}  // namespace

#include <chrome/browser/ui/tabs/tab_strip_model.cc>  // IWYU pragma: export

std::unique_ptr<DetachedTabCollection>
TabStripModel::DetachTreeTabNodeForInsertion(
    tabs::TabInterface* subtree_root_tab,
    const std::vector<tabs::TabInterface*>& moving_tabs) {
  auto* tree_node_collection =
      tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(
          subtree_root_tab);
  CHECK(tree_node_collection);

  std::optional<int> active_index_in_collection;
  int index = 0;
  for (tabs::TabInterface* tab : *tree_node_collection) {
    if (tab->IsActivated()) {
      active_index_in_collection = index;
      break;
    }
    index++;
  }

  // Unregister every node in this subtree from the source window's
  // TreeTabModel before it is physically removed.
  contents_data_->WillDetachTreeTabNodeSubtree(*tree_node_collection);

  std::unique_ptr<tabs::TabCollection> detached_collection =
      DetachTabCollectionImpl(
          tree_node_collection,
          base::BindOnce(&tabs::TabStripCollection::RemoveTabCollection,
                         base::Unretained(contents_data_.get()),
                         tree_node_collection),
          base::DoNothing());

  return std::make_unique<DetachedTabCollection>(
      base::WrapUnique(static_cast<tabs::TreeTabNodeTabCollection*>(
          detached_collection.release())),
      active_index_in_collection, /*pinned_=*/false);
}

gfx::Range TabStripModel::InsertDetachedTreeTabNodeAt(
    std::unique_ptr<DetachedTabCollection> tree_node,
    int index) {
  CHECK(std::holds_alternative<std::unique_ptr<tabs::TreeTabNodeTabCollection>>(
      tree_node->collection_));

  std::unique_ptr<tabs::TreeTabNodeTabCollection>
      tree_node_collection_unique_ptr =
          std::move(std::get<std::unique_ptr<tabs::TreeTabNodeTabCollection>>(
              tree_node->collection_));
  tabs::TreeTabNodeTabCollection* tree_node_collection =
      tree_node_collection_unique_ptr.get();

  // Notify tab is added to model.
  for (tabs::TabInterface* tab : *tree_node_collection) {
    static_cast<tabs::TabModel*>(tab)->OnAddedToModel(this);
  }

  index = ConstrainInsertionIndex(index, false);

  // Registers the subtree with the destination window's TreeTabModel (via
  // BraveTabStripCollection::InsertDetachedTreeTabNode ->
  // DidAttachTreeTabNodeSubtree) only after the physical insertion below,
  // since TreeTabModel::AddTreeTabNode needs the node's post-move parent
  // chain to compute the collapsed-ancestor cache correctly.
  return InsertDetachedCollectionImpl(
      tree_node_collection, tree_node->active_index_,
      base::BindOnce(&tabs::TabStripCollection::InsertDetachedTreeTabNode,
                     base::Unretained(contents_data_.get()),
                     std::move(tree_node_collection_unique_ptr), index),
      base::DoNothing());
}
