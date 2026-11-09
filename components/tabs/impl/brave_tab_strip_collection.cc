// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection.h"

#include <algorithm>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/map_util.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/pinned_tab_collection.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_group_tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "components/tabs/public/tab_strip_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace tabs {

// The collection hierarchy with tree tab node collections unwrapped, which is
// what observers(TabCollectionObserver) should know of. The reasons for
// hiding tree tab node are:
//  1. TabCollectionObservers are mostly upstream code. They should not be
//     notified with TreeTabNodeCollection, so that we don't have to customize
//     all observers for this special type.
//  2. This is also true for TabCollectionNode, which inherits
//     TabCollectionObserver and implements a view model vertical tab strip.
//     As vertical tab strip only need indentation, we can keep the view model
//     unaware of tree tab nodes.
//
// If |collection_to_notify| is given, this is the hierarchy that observers know
// of. It's shared by nested operations of the delegate, and when the last one
// releases it, observers are notified of the changes since then.
class BraveTabStripCollection::UnwrappedHierarchy
    : public base::RefCounted<UnwrappedHierarchy> {
 public:
  explicit UnwrappedHierarchy(
      const TabCollection& root,
      BraveTabStripCollection* collection_to_notify = nullptr)
      : collection_to_notify_(collection_to_notify) {
    Add(root);
  }
  UnwrappedHierarchy(const UnwrappedHierarchy&) = delete;
  UnwrappedHierarchy& operator=(const UnwrappedHierarchy&) = delete;

  base::WeakPtr<UnwrappedHierarchy> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // Makes this the hierarchy of |root| as it is.
  void Reset(const TabCollection& root) {
    handle_to_children_.clear();
    handle_to_parent_.clear();
    Add(root);
  }

  bool Contains(const TabCollectionNodeHandle& handle) const {
    return handle_to_parent_.contains(handle);
  }

  const std::vector<TabCollectionNodeHandle>& GetChildren(
      const TabCollectionHandle& collection) const {
    const auto* children = base::FindOrNull(handle_to_children_, collection);
    CHECK(children);
    return *children;
  }

  TabCollectionHandle GetParent(const TabCollectionNodeHandle& handle) const {
    const auto* parent = base::FindOrNull(handle_to_parent_, handle);
    CHECK(parent);
    return *parent;
  }

  size_t GetIndex(const TabCollectionNodeHandle& handle) const {
    const auto& siblings = GetChildren(GetParent(handle));
    const auto it = std::ranges::find(siblings, handle);
    CHECK(it != siblings.end());
    return static_cast<size_t>(it - siblings.begin());
  }

  // Returns the index right after |predecessor| in |parent|, or 0 if it's
  // null.
  size_t GetIndexAfter(const TabCollectionHandle& parent,
                       const TabCollectionNodeHandle* predecessor) const {
    if (!predecessor) {
      return 0;
    }
    CHECK(GetParent(*predecessor) == parent);
    return GetIndex(*predecessor) + 1;
  }

  bool IsChildOf(const TabCollectionNodeHandle& handle,
                 const TabCollectionHandle& parent) const {
    const auto* actual_parent = base::FindOrNull(handle_to_parent_, handle);
    return actual_parent && *actual_parent == parent;
  }

  // Returns true if |handle| is where |current| has it: right after
  // |predecessor|, or first if it's null. The siblings that |current| doesn't
  // have in the same parent are skipped, as they'll be moved out or removed.
  bool IsPlacedAfter(const TabCollectionNodeHandle& handle,
                     const TabCollectionNodeHandle* predecessor,
                     const UnwrappedHierarchy& current) const {
    const TabCollectionHandle parent = current.GetParent(handle);
    if (!IsChildOf(handle, parent)) {
      return false;
    }

    // Get old siblings from this. Some of them might not be in the current
    // hierarchy anymore, as they might have been moved or removed.
    const auto& old_siblings = GetChildren(parent);

    // The closest sibling before |handle| that stays in |parent|.
    for (size_t i = GetIndex(handle) - 1; i < old_siblings.size(); --i) {
      if (current.IsChildOf(old_siblings[i], parent)) {
        // We found a sibling before |handle| that was in both |this| and
        // |current| hierarchy.
        return predecessor && old_siblings[i] == *predecessor;
      }
    }

    // No preceding sibling found that stays in the current parent.
    return !predecessor;
  }

  // Returns true if any descendant of |collection| in |other| is in this.
  bool ContainsAnyDescendant(const UnwrappedHierarchy& other,
                             const TabCollectionHandle& collection) const {
    return std::ranges::any_of(
        other.GetChildren(collection),
        [&](const TabCollectionNodeHandle& child) {
          const auto* child_collection =
              std::get_if<TabCollectionHandle>(&child);
          return Contains(child) ||
                 (child_collection &&
                  ContainsAnyDescendant(other, *child_collection));
        });
  }

  // Inserts |handle| at |index| of |parent|. If it's a collection, it keeps
  // its descendants, if any.
  void Insert(const TabCollectionNodeHandle& handle,
              const TabCollectionHandle& parent,
              size_t index) {
    const auto it = handle_to_children_.find(parent);
    CHECK(it != handle_to_children_.end());
    CHECK_LE(index, it->second.size());
    it->second.insert(it->second.begin() + index, handle);
    handle_to_parent_[handle] = parent;
    if (const auto* collection = std::get_if<TabCollectionHandle>(&handle)) {
      handle_to_children_.try_emplace(*collection);
    }
  }

  // Copies the descendants of |collection| from |other|.
  void CopyDescendants(const UnwrappedHierarchy& other,
                       const TabCollectionHandle& collection) {
    const auto& other_children = other.GetChildren(collection);
    handle_to_children_[collection] = other_children;
    for (const auto& child : other_children) {
      handle_to_parent_[child] = collection;
      if (const auto* child_collection =
              std::get_if<TabCollectionHandle>(&child)) {
        CopyDescendants(other, *child_collection);
      }
    }
  }

  // Takes |handle| out of its parent, keeping its descendants.
  void Detach(const TabCollectionNodeHandle& handle) {
    const auto it = handle_to_children_.find(GetParent(handle));
    CHECK(it != handle_to_children_.end());
    std::erase(it->second, handle);
    handle_to_parent_.erase(handle);
  }

  // Removes |handle| along with its descendants.
  void Remove(const TabCollectionNodeHandle& handle) {
    Detach(handle);
    RemoveDescendants(handle);
  }

 private:
  friend class base::RefCounted<UnwrappedHierarchy>;

  ~UnwrappedHierarchy() {
    if (collection_to_notify_) {
      collection_to_notify_->NotifyUnwrappedChanges();
    }
  }

  void Add(const TabCollection& collection) {
    std::vector<TabCollectionNodeHandle> handles;
    AddUnwrappedChildren(collection, collection.GetHandle(), handles);
    handle_to_children_[collection.GetHandle()] = std::move(handles);
  }

  // Adds the children of |collection| as the children of |parent|, replacing
  // tree tab node collections with their children.
  void AddUnwrappedChildren(const TabCollection& collection,
                            const TabCollectionHandle& parent,
                            std::vector<TabCollectionNodeHandle>& handles) {
    for (const Child& child : collection.GetChildren()) {
      if (const auto* child_collection =
              std::get_if<std::unique_ptr<TabCollection>>(&child)) {
        if ((*child_collection)->type() == TabCollection::Type::TREE_NODE) {
          AddUnwrappedChildren(**child_collection, parent, handles);
          continue;
        }

        const TabCollectionHandle child_collection_handle =
            (*child_collection)->GetHandle();
        handles.push_back(child_collection_handle);
        handle_to_parent_[child_collection_handle] = parent;

        Add(**child_collection);
        continue;
      }

      const TabHandle tab_handle =
          std::get<ScopedTab>(child).get()->GetHandle();
      handles.push_back(tab_handle);
      handle_to_parent_[tab_handle] = parent;
    }
  }

  void RemoveDescendants(const TabCollectionNodeHandle& handle) {
    const auto* collection = std::get_if<TabCollectionHandle>(&handle);
    if (!collection) {
      return;
    }

    const auto it = handle_to_children_.find(*collection);
    CHECK(it != handle_to_children_.end());

    const std::vector<TabCollectionNodeHandle> descendants =
        std::move(it->second);
    handle_to_children_.erase(it);
    for (const auto& descendant : descendants) {
      handle_to_parent_.erase(descendant);
      RemoveDescendants(descendant);
    }
  }

  absl::flat_hash_map<TabCollectionHandle, std::vector<TabCollectionNodeHandle>>
      handle_to_children_;
  absl::flat_hash_map<TabCollectionNodeHandle, TabCollectionHandle>
      handle_to_parent_;
  const raw_ptr<BraveTabStripCollection> collection_to_notify_;
  base::WeakPtrFactory<UnwrappedHierarchy> weak_factory_{this};
};

// We disable |send_notifications_immediately| in order to be aligned with
// the original TabStripCollection creation in TabStripModel.
BraveTabStripCollection::BraveTabStripCollection()
    : TabStripCollection(/*send_notifications_immediately=*/false) {}

BraveTabStripCollection::~BraveTabStripCollection() = default;

scoped_refptr<BraveTabStripCollection::UnwrappedHierarchy>
BraveTabStripCollection::ScopeUnwrappedNotifications() {
  // Nested operations share the outermost one.
  if (unwrapped_hierarchy_snapshot_) {
    return base::WrapRefCounted(unwrapped_hierarchy_snapshot_.get());
  }

  auto snapshot = base::MakeRefCounted<UnwrappedHierarchy>(
      /*root=*/*this, /*collection_to_notify=*/this);
  unwrapped_hierarchy_snapshot_ = snapshot->GetWeakPtr();
  return snapshot;
}

void BraveTabStripCollection::SetDelegate(
    std::unique_ptr<BraveTabStripCollectionDelegate> delegate) {
  CHECK_NE(delegate_, delegate);
  delegate_ = std::move(delegate);
}

tabs::TabCollection* BraveTabStripCollection::GetParentCollection(
    TabInterface* tab,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) const {
  return tab->GetParentCollection(GetPassKey());
}

const ChildrenVector& BraveTabStripCollection::GetChildrenForDelegate(
    const TabCollection& collection,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) const {
  return GetChildrenStatic(collection);
}

void BraveTabStripCollection::AddTabRecursive(
    ScopedTab tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  CHECK(tab);
  CHECK_LE(index, TabCountRecursive());
  if (new_group_id.has_value()) {
    CHECK(GetTabGroupCollection(new_group_id.value()));
  }

  const TabCollection::Position position = GetInsertionDetails(
      static_cast<int>(index), new_pinned_state, new_group_id);
  position.parent_handle.Get()->AddTab(std::move(tab), position.index);
}

ScopedTab BraveTabStripCollection::RemoveTabAtIndexRecursive(
    size_t index,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  TabInterface* tab = GetTabAtIndexRecursive(index);
  TabCollection* parent = tab->GetParentCollection(GetPassKey());
  CHECK(parent);

  if (parent->type() == TabCollection::Type::TREE_NODE) {
    ScopedTab removed_tab = parent->MaybeRemoveTab(tab);
    if (TabCollection* grand_parent = parent->GetParentCollection();
        grand_parent && grand_parent->type() == TabCollection::Type::GROUP &&
        grand_parent->TabCountRecursive() == 0) {
      std::ignore = RemoveTabCollectionSilently(grand_parent);
    }
    return removed_tab;
  }

  // If it is the only tab in the group remove the collection.
  if (parent != unpinned_collection_ && parent != pinned_collection_ &&
      parent->TabCountRecursive() == 1) {
    std::ignore = RemoveTabCollectionSilently(parent);
  }
  return parent->MaybeRemoveTab(tab);
}

void BraveTabStripCollection::AddTabCollectionAtPosition(
    std::unique_ptr<TabCollection> collection,
    const TabCollection::Position& position,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  AddTabCollectionSilently(std::move(collection), position);
}

void BraveTabStripCollection::AddTabRecursive(
    ScopedTab tab,
    size_t index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    TabInterface* opener) {
  if (ShouldDelegate()) {
    auto notifications = ScopeUnwrappedNotifications();
    delegate_->AddTabRecursive(std::move(tab), index, new_group_id,
                               new_pinned_state, opener);
    return;
  }

  TabStripCollection::AddTabRecursive(std::move(tab), index, new_group_id,
                                      new_pinned_state);
}

void BraveTabStripCollection::MoveTabsRecursive(
    const std::vector<int>& tab_indices,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const TabCollection::TypeEnumSet retain_collection_types) {
  if (ShouldDelegate()) {
    auto notifications = ScopeUnwrappedNotifications();
    delegate_->MoveTabsRecursive(tab_indices, destination_index, new_group_id,
                                 new_pinned_state, retain_collection_types);
    return;
  }

  TabStripCollection::MoveTabsRecursive(tab_indices, destination_index,
                                        new_group_id, new_pinned_state,
                                        retain_collection_types);
}

ScopedTab BraveTabStripCollection::RemoveTabAtIndexRecursive(size_t index) {
  if (ShouldDelegate()) {
    auto notifications = ScopeUnwrappedNotifications();
    return delegate_->RemoveTabAtIndexRecursive(index);
  }

  return TabStripCollection::RemoveTabAtIndexRecursive(index);
}

void BraveTabStripCollection::InsertTabCollectionAt(
    std::unique_ptr<TabCollection> collection,
    int index,
    bool pinned,
    std::optional<tab_groups::TabGroupId> parent_group) {
  if (ShouldDelegate()) {
    auto notifications = ScopeUnwrappedNotifications();
    delegate_->InsertTabCollectionAt(std::move(collection), index, pinned,
                                     parent_group);
    return;
  }

  TabStripCollection::InsertTabCollectionAt(std::move(collection), index,
                                            pinned, parent_group);
}

void BraveTabStripCollection::InsertTabCollectionAt(
    std::unique_ptr<TabCollection> collection,
    int index,
    bool pinned,
    std::optional<tab_groups::TabGroupId> parent_group,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  AddTabCollectionSilently(std::move(collection),
                           GetInsertionDetails(index, pinned, parent_group));
}

std::unique_ptr<TabGroupTabCollection>
BraveTabStripCollection::PopDetachedGroupCollectionForDelegate(
    tab_groups::TabGroupId group_id,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  return PopDetachedGroupCollection(group_id);
}

void BraveTabStripCollection::CreateSplit(
    split_tabs::SplitTabId split_id,
    const std::vector<TabInterface*>& tabs,
    split_tabs::SplitTabVisualData visual_data) {
  if (ShouldDelegate()) {
    bool handled = false;
    {
      auto notifications = ScopeUnwrappedNotifications();
      handled = delegate_->CreateSplit(split_id, tabs, visual_data);
    }
    if (handled) {
      return;
    }
  }
  TabStripCollection::CreateSplit(split_id, tabs, visual_data);
}

void BraveTabStripCollection::MoveTabsRecursiveForDelegate(
    const std::vector<int>& tab_indices,
    size_t destination_index,
    std::optional<tab_groups::TabGroupId> new_group_id,
    bool new_pinned_state,
    const TabCollection::TypeEnumSet retain_collection_types,
    base::PassKey<BraveTabStripCollectionDelegate> pass_key) {
  // TabStripCollection notifies the moves by itself. The delegate only moves
  // tabs within a group with this, where there's no tree tab node collection,
  // so observers can take the notifications as they are once they know of the
  // changes so far.
  if (unwrapped_hierarchy_snapshot_) {
    NotifyUnwrappedChanges();
  }
  TabStripCollection::MoveTabsRecursive(tab_indices, destination_index,
                                        new_group_id, new_pinned_state,
                                        retain_collection_types);
  if (unwrapped_hierarchy_snapshot_) {
    unwrapped_hierarchy_snapshot_->Reset(*this);
  }
}

void BraveTabStripCollection::MoveCollectionAndNotify(TabCollection* collection,
                                                      TabCollection* new_parent,
                                                      size_t index) {
  auto notifications = ScopeUnwrappedNotifications();
  TabCollection* old_parent = collection->GetParentCollection();
  new_parent->AddCollection(old_parent->MaybeRemoveCollection(collection),
                            index);
}

void BraveTabStripCollection::Unsplit(split_tabs::SplitTabId split_id) {
  if (ShouldDelegate()) {
    bool handled = false;
    {
      auto notifications = ScopeUnwrappedNotifications();
      handled = delegate_->Unsplit(split_id);
    }
    if (handled) {
      return;
    }
  }
  TabStripCollection::Unsplit(split_id);
}

void BraveTabStripCollection::AddCollectionMapping(
    TabCollection* root_collection) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    TabStripCollection::AddCollectionMapping(
        delegate_->GetCollectionForMapping(root_collection));
    return;
  }

  TabStripCollection::AddCollectionMapping(root_collection);
}

void BraveTabStripCollection::RemoveCollectionMapping(
    TabCollection* root_collection) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    TabStripCollection::RemoveCollectionMapping(
        delegate_->GetCollectionForMapping(root_collection));
    return;
  }

  TabStripCollection::RemoveCollectionMapping(root_collection);
}

const tree_tab::TreeTabNodeId*
BraveTabStripCollection::GetTreeTabNodeIdForGroup(
    tab_groups::TabGroupId group_id) const {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    return delegate_->GetTreeTabNodeIdForGroup(group_id);
  }

  return nullptr;
}

void BraveTabStripCollection::PrepareTreeTabNodesForBatchDetach(
    const std::vector<TabInterface*>& moving_tabs) {
  if (ShouldDelegate()) {
    auto notifications = ScopeUnwrappedNotifications();
    delegate_->PrepareTreeTabNodesForBatchDetach(moving_tabs);
  }
}

bool BraveTabStripCollection::ShouldDetachAsTreeSubtreeRoot(
    TabInterface* tab,
    const std::vector<TabInterface*>& moving_tabs) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    return delegate_->ShouldDetachAsTreeSubtreeRoot(tab, moving_tabs);
  }
  return false;
}

void BraveTabStripCollection::WillDetachTreeTabNodeSubtree(
    TreeTabNodeTabCollection& subtree_root) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    delegate_->WillDetachTreeTabNodeSubtree(subtree_root);
  }
}

void BraveTabStripCollection::DidAttachTreeTabNodeSubtree(
    TreeTabNodeTabCollection& subtree_root) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    delegate_->DidAttachTreeTabNodeSubtree(subtree_root);
  }
}

void BraveTabStripCollection::InsertDetachedTreeTabNode(
    std::unique_ptr<TabCollection> collection,
    int index) {
  // Insert the already tree-shaped collection directly, bypassing the
  // delegate's InsertTabCollectionAt (which only knows how to wrap raw
  // SPLIT/GROUP collections in a fresh tree node, not re-attach an existing
  // TreeTabNodeTabCollection subtree).
  auto* tree_node_ptr =
      static_cast<TreeTabNodeTabCollection*>(collection.get());
  if (!ShouldDelegate()) {
    TabStripCollection::InsertTabCollectionAt(std::move(collection), index,
                                              /*pinned=*/false,
                                              /*parent_group=*/std::nullopt);
    return;
  }

  auto notifications = ScopeUnwrappedNotifications();
  AddTabCollectionSilently(
      std::move(collection),
      GetInsertionDetails(index, /*pinned=*/false, /*group=*/std::nullopt));
  delegate_->DidAttachTreeTabNodeSubtree(*tree_node_ptr);
}

std::unique_ptr<TabCollection> BraveTabStripCollection::RemoveTabCollection(
    TabCollection* collection) {
  if (!ShouldDelegate()) {
    return TabStripCollection::RemoveTabCollection(collection);
  }

  // e.g. A group wrapped by a tree tab node collection is detached to be
  // dragged to another window.
  auto notifications = ScopeUnwrappedNotifications();
  std::unique_ptr<TabCollection> removed_collection =
      RemoveTabCollectionSilently(collection);
  if (collection->type() == TabCollection::Type::GROUP) {
    return PopDetachedGroupCollection(
        static_cast<TabGroupTabCollection*>(collection)->GetTabGroupId());
  }
  CHECK(removed_collection);
  return removed_collection;
}

bool BraveTabStripCollection::ShouldDelegate() const {
  return delegate_ && delegate_->ShouldHandleTabManipulation();
}

void BraveTabStripCollection::AddTabCollectionSilently(
    std::unique_ptr<TabCollection> collection,
    const TabCollection::Position& position) {
  TabCollection* parent = position.parent_handle.Get();
  CHECK(parent);
  AddCollectionMapping(collection.get());
  parent->AddCollection(std::move(collection), position.index);
}

std::unique_ptr<TabCollection>
BraveTabStripCollection::RemoveTabCollectionSilently(
    TabCollection* collection) {
  TabCollection* parent = collection->GetParentCollection();
  CHECK(parent);
  RemoveCollectionMapping(collection);
  std::unique_ptr<TabCollection> removed_collection =
      parent->MaybeRemoveCollection(collection);
  CHECK(removed_collection);

  // As TabStripCollection does, keep the group until TabStripModel closes it.
  if (removed_collection->type() == TabCollection::Type::GROUP) {
    CreateTabGroup(base::WrapUnique(
        static_cast<TabGroupTabCollection*>(removed_collection.release())));
  }
  return removed_collection;
}

void BraveTabStripCollection::NotifyUnwrappedChanges() {
  CHECK(unwrapped_hierarchy_snapshot_);
  const auto current = base::MakeRefCounted<UnwrappedHierarchy>(/*root=*/*this);
  // Places the remaining ones first, so that removed collections don't take
  // them along, e.g. the tabs of an unsplit split.
  NotifyPlacedNodes(*current, GetHandle());
  NotifyRemovedNodes(*current, GetHandle());
}

void BraveTabStripCollection::NotifyRemovedNodes(
    const UnwrappedHierarchy& current,
    const TabCollectionHandle& parent) {
  // Copied, as removing a child changes them.
  const std::vector<TabCollectionNodeHandle> children =
      unwrapped_hierarchy_snapshot_->GetChildren(parent);
  for (const auto& child : children) {
    if (current.Contains(child)) {
      if (const auto* collection = std::get_if<TabCollectionHandle>(&child)) {
        NotifyRemovedNodes(current, *collection);
      }
      continue;
    }

    // Its remaining descendants, if any, are removed as well. |parent| is
    // still in the hierarchy, as it's in |current|.
    const size_t index = unwrapped_hierarchy_snapshot_->GetIndex(child);
    unwrapped_hierarchy_snapshot_->Remove(child);
    parent.Get()->NotifyOnChildrenRemoved(GetPassKey(), {parent, index},
                                          {child},
                                          /*stop_notification_root=*/nullptr);
  }
}

void BraveTabStripCollection::NotifyPlacedNodes(
    const UnwrappedHierarchy& current,
    const TabCollectionHandle& parent) {
  // Places the children in order, each right after the one before it, which
  // is in place already. Observers still know of the children that are moved
  // out or removed later, so the indices are of their hierarchy.
  TabCollection* parent_collection = parent.Get();
  const auto& children = current.GetChildren(parent);
  for (size_t i = 0; i < children.size(); ++i) {
    const TabCollectionNodeHandle& child = children[i];
    const TabCollectionNodeHandle* predecessor = i ? &children[i - 1] : nullptr;
    const auto* collection = std::get_if<TabCollectionHandle>(&child);

    if (!unwrapped_hierarchy_snapshot_->Contains(child)) {
      // Read the whole subtree of a new collection, as Chromium does for
      // detached collections, unless observers know some of it already.
      // Those are moved into it instead.
      const bool insert_from_detached =
          collection && collection->Get()->TabCountRecursive() > 0 &&
          !unwrapped_hierarchy_snapshot_->ContainsAnyDescendant(current,
                                                                *collection);
      const size_t index =
          unwrapped_hierarchy_snapshot_->GetIndexAfter(parent, predecessor);
      unwrapped_hierarchy_snapshot_->Insert(child, parent, index);
      if (insert_from_detached) {
        unwrapped_hierarchy_snapshot_->CopyDescendants(current, *collection);
      }
      parent_collection->NotifyOnChildrenAdded(
          GetPassKey(), {child}, {parent, index},
          /*stop_notification_root=*/nullptr, insert_from_detached);
      if (insert_from_detached) {
        continue;
      }
    } else if (!unwrapped_hierarchy_snapshot_->IsPlacedAfter(child, predecessor,
                                                             current)) {
      const TabCollectionHandle from_parent =
          unwrapped_hierarchy_snapshot_->GetParent(child);
      const size_t from_index = unwrapped_hierarchy_snapshot_->GetIndex(child);
      // The index is of after |child| is taken out, as observers expect.
      unwrapped_hierarchy_snapshot_->Detach(child);
      const size_t index =
          unwrapped_hierarchy_snapshot_->GetIndexAfter(parent, predecessor);
      unwrapped_hierarchy_snapshot_->Insert(child, parent, index);
      parent_collection->NotifyOnChildMoved(
          GetPassKey(), child, {from_parent, from_index}, {parent, index},
          /*stop_notification_root=*/nullptr);
    }

    if (collection) {
      NotifyPlacedNodes(current, *collection);
    }
  }
}

}  // namespace tabs
