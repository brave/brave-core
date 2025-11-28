# Tree Tabs Architecture

## Overview

This document explores the implementation of "Tree Tabs" - a feature that allows
users to see their tabs organized in a hierarchical tree structure based on
their opener relationships. The tree tabs implementation builds upon the
existing **Group Tabs** architecture in Chromium, extending the tab collection
framework to support hierarchical tab organization.

## Foundation: Group Tabs Architecture

Before diving into tree tabs, it's essential to understand the existing Group Tabs architecture that serves as the foundation.

### Core Components

#### 1. TabCollection Hierarchy

The [tab collection](https://source.chromium.org/chromium/chromium/src/+/main:components/tabs/public/tab_collection.h;drc=ecd02bc4ab0da9efc6f1765f4f46fb46252f3167) framework is designed as a hierarchical system that can contain both tabs and other collections:

```cpp
class TabCollection {
public:
  // Type describes the various kinds of tab collections:
  // - TABSTRIP:  The main container for tabs in a browser window.
  // - PINNED:    A container for pinned tabs.
  // - UNPINNED:  A container for unpinned tabs.
  // - GROUP:     A container to grouped tabs.
  // - SPLIT:     A container for split tabs.
  enum class Type { TABSTRIP, PINNED, UNPINNED, GROUP, SPLIT };
  
  // Hierarchical structure support
  TabInterface* AddTab(std::unique_ptr<TabInterface> tab, size_t index);
  T* AddCollection(std::unique_ptr<T> collection, size_t index);
  ...
};
```

The TabCollection provides recursive representation of groups of tabs. 
So examplary structure would be like this:

```
TabStripCollection
 ├── PinnedCollection
 │    ├── Tab 1
 │    └── Tab 2
 │
 └── UnpinnedCollection
      ├── GroupTabCollection (Group A)
      │    ├── Tab 3
      │    └── Tab 4
      ├── SplitTabCollection
      │    ├── Tab 6
      │    └── Tab 7
      └── Tab 5
```

The TabCollection also **manages ownership** of its tabs and child collections, 
ensuring proper lifecycle management. So **only TabStripModel** can directly
manipulate TabCollection hierarchy.

[**TabStripModel** owns **TabStripCollection** with the name `contents_data_`](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/tab_strip_model.h;l=1379;drc=3170b88bbe6677b6b465c81fe18f7a4cb93bc67c),
and access and manipulates tabs and collections through it. So we can think of
[`TabStripCollection`](https://source.chromium.org/chromium/chromium/src/+/main:components/tabs/public/tab_strip_collection.h;drc=a6e87177f32db34e28997e3f2d9fc67968e58fab) as the **central interface of tab management** for 
TabStripModel.


#### 2. TabGroup as Metadata Container

[**TabGroupTabCollection**](https://source.chromium.org/chromium/chromium/src/+/main:components/tabs/public/tab_group_tab_collection.h;drc=6c7213c72cc5e42c881d0ae57127ffe550963893) is a specialized TabCollection that manages grouped
tabs. But it's focusing on managing the lifecycle of tabs within the group, while
the actual group metadata is stored in a separate **TabGroup** class.

```cpp
class TabGroupTabCollection : public TabCollection {
  // OWNS the TabGroup metadata
  std::unique_ptr<TabGroup> group_;
};

```


[**TabGroup**](https://source.chromium.org/chromium/chromium/src/+/main:components/tabs/public/tab_group.h;drc=fcc336e81a365fd858cae859059b29be8f995427) contains all group-related metadata:

```cpp
class TabGroup {
  // All group metadata is stored here

  // Color, title, collapsed state, etc.
  tab_groups::TabGroupVisualData visual_data_;

  // Also provides the accessors for TabInterface within the group
  tabs::TabInterface* GetFirstTab() const;
  tabs::TabInterface* GetLastTab() const;
};
```

The important point here is that **TabGroupTabCollection** is not exposed to
UI components except **TabStripModel**. Instead, UI components access group 
metadata through **TabGroupModel**.

[**BrowserTabStripController**](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/views/tabs/browser_tab_strip_controller.cc;l=757;drc=0fe57d75274de3140505b20e948a6d9fca1ffb68) demonstrates how UI components access group metadata:

```cpp
bool BrowserTabStripController::IsGroupCollapsed(
    const tab_groups::TabGroupId& group) const {
  return model_->group_model()->ContainsTabGroup(group) &&
         model_->group_model()
             ->GetTabGroup(group)
             ->visual_data()
             ->is_collapsed();
}
```

#### 4. TabGroupModel as Registry

[**TabGroupModel**](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/tab_group_model.h;drc=fb8950cdd1c1de2b7d7fcc92ebbc5c84c03107a9) only contains TabGroup references for access. [**TabGroupModel**
is also owned by **TabStripModel**](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/tab_strip_model.h;l=1382;drc=3170b88bbe6677b6b465c81fe18f7a4cb93bc67c).

```cpp
class TabGroupModel {
  // Registry pointing to TabGroups (doesn't own them)
  std::map<tab_groups::TabGroupId, raw_ptr<TabGroup>> groups_;
  
  // These will be called by TabStrripModel when it manipulates
  // TabGroupTabCollection.
  void AddTabGroup(TabGroup* group, base::PassKey<TabStripModel>);
  void RemoveTabGroup(const tab_groups::TabGroupId& id, base::PassKey<TabStripModel>);
};
```

**Example: How TabStripModel creates a group**  
When a user creates a new tab group, [TabStripModel::AddToNewGroup](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/tab_strip_model.cc;l=4138;drc=0fe57d75274de3140505b20e948a6d9fca1ffb68) demonstrates the integration pattern:

```cpp
void TabStripModel::AddToNewGroup(const std::vector<int>& indices) {
  ...

  // 1. Create TabGroupTabCollection with metadata
    std::unique_ptr<tabs::TabGroupTabCollection> group_collection =
      std::make_unique<tabs::TabGroupTabCollection>(
          factory, new_group,
          tab_groups::TabGroupVisualData(
              std::u16string(),
              group_model_->GetNextColor(base::PassKey<TabStripModel>())));

  // 2. Register in TabGroupModel for UI access
  group_model_->AddTabGroup(group_collection->GetTabGroup(),
                            base::PassKey<TabStripModel>());

  // 3. Move group collection into the collection
  contents_data_->CreateTabGroup(std::move(group_collection));

  ...
}
```

This pattern shows how ownership flows: TabCollection → TabGroup → TabGroupModel registry.

**Critical Access Pattern**: 
- **TabCollection system** manages ownership
- **UI components** (except TabStripModel) must access **TabGroup** via **TabGroupModel**, never **TabGroupTabCollection** directly

**Example: UI accessing group metadata**  
[BrowserTabStripController](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/views/tabs/browser_tab_strip_controller.cc;l=723;drc=b2ea91862833a9b13ff555010327c6b65e1820e1) demonstrates proper access through TabGroupModel:

```cpp
TabGroup* BrowserTabStripController::GetTabGroup(
    const tab_groups::TabGroupId& group_id) const {
  // It's getting TabGroup through TabGroupModel, not TabGroupTabCollection.
  return model_->group_model()->GetTabGroup(group_id);
}

std::u16string BrowserTabStripController::GetGroupTitle(
    const tab_groups::TabGroupId& group) const {
  // Convenient accessor for group title
  return model_->group_model()->GetTabGroup(group)->visual_data()->title();
}

void BrowserTabStripController::SetVisualDataForGroup(
    const tab_groups::TabGroupId& group,
    const tab_groups::TabGroupVisualData& visual_data) {
  // In case of updating group visuals, TabStripModel provides the setter. But
  // TabStripModel also access the group via TabGroupModel.
  model_->ChangeTabGroupVisuals(group, visual_data);
}
```

#### 5. TabGroupChanged Notifications
All changes to tab groups will be [notified through **TabStripModelObserver**](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/tab_strip_model_observer.h;l=549;drc=3170b88bbe6677b6b465c81fe18f7a4cb93bc67c),
typically observed by UI components:

```cpp
class TabStripModelObserver {
  virtual void OnTabGroupChanged(const TabGroupChange& change);
};
```

Change details are encapsulated in the [**TabGroupChange**](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/tab_strip_model_observer.h;l=263;drc=3170b88bbe6677b6b465c81fe18f7a4cb93bc67c) struct:

```cpp
struct TabGroupChange {
  // A group is created when the first tab is added to it and closed when the
  // last tab is removed from it. Whenever the set of tabs in the group changes,
  // a kContentsChange event is fired. Whenever the group's visual data changes,
  // such as its title or color, a kVisualsChange event is fired. Whenever the
  // group is moved by interacting with its header, a kMoved event is fired.
  enum Type {
    kCreated,
    kEditorOpened,
    kVisualsChanged,
    kMoved,
    kClosed
  };

  // Base class for all changes. Similar to TabStripModelChange::Delta.
  struct Delta { virtual ~Delta() = default; };
  struct VisualsChange : public Delta { ... };
  struct CreateChange : public Delta { ...  };
```

**Example: UI observing group changes**  
[BrowserTabStripController](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/views/tabs/browser_tab_strip_controller.cc;l=922;drc=b2ea91862833a9b13ff555010327c6b65e1820e1) demonstrates how UI components react to group changes:

```cpp
void BrowserTabStripController::OnTabGroupChanged(
    const TabGroupChange& change) {
  // BrowserTabStripController is a central controller in favor of MVC model.
  // That means it's responsible for delivering model update requests from TabStrip(View)
  // to TabStripModel and propagating model changes back to TabStrip.
  // In this case, you can see that the controller is propagating group changes to the TabStrip
  // by calling proper methods
  switch (change.type) {
    case TabGroupChange::kCreated: {
      tabstrip_->OnGroupCreated(change.group);
      ...
      break;
    }
    case TabGroupChange::kEditorOpened: {
      tabstrip_->OnGroupEditorOpened(change.group);
      break;
    }
    case TabGroupChange::kVisualsChanged: {
      ...
      tabstrip_->OnGroupVisualsChanged(change.group, old_visuals, new_visuals);
      break;
    }
    case TabGroupChange::kMoved: {
      tabstrip_->OnGroupMoved(change.group);
      break;
    }
    case TabGroupChange::kClosed: {
      tabstrip_->OnGroupClosed(change.group);
      break;
    }
  }
}
```

## Tree Tabs Implementation

Building on this foundation, tree tabs extend the collection framework with 
hierarchical tab relationships based on opener connections.

### Core Components

#### 1. TreeTabNodeTabCollection as Central Management System

**TreeTabNodeTabCollection** is the central component managing the hierarchical
tree system. And **TreeTabNode** is the metadata container for tree-related
information. This will be owned by **TreeTabNodeTabCollection**.


```cpp
class TabCollection {
public:
  enum class Type { TABSTRIP, PINNED, UNPINNED, GROUP, SPLIT,
                     TREE_TAB_NODE  // New type for tree tab nodes
                  };
}
```

```cpp
class TreeTabNodeTabCollection : public TabCollection {
  raw_ptr<TabInterface> current_tab_;     // The tab this node represents
  std::unique_ptr<TreeTabNode> node_;     // OWNS the TreeTabNode metadata
  
  // Called when tree tab is truned on or off to build/flatten tree structure
  static void BuildTreeTabs(TabCollection& root, /* callbacks */);
  static void FlattenTreeTabs(TabCollection& root, /* callbacks */);
  
  // Hierarchy navigation  
  TreeTabNodeTabCollection* GetTopLevelAncestor();
  std::vector<std::variant<TabInterface*, TabCollection*>> GetTreeNodeChildren();

  // A class that represents metadata about the tree tab node.
  std::unique_ptr<TreeTabNode> node_;
};
```


The **TreeTabNodeTabCollection** owns one tab(or groups/split if needed in future)
and other **TreeTabNodeTabCollection** as its children, forming a tree structure.
So we would have structure of tree tabs like this when it's enabled:

```TabStripCollection
 ├── PinnedCollection
 │    ├── Tab 1
 │    └── Tab 2
 │
 └── UnpinnedCollection
      ├── TreeTabNodeTabCollection
      │    ├── Tab3
      │    ├── TreeTabNodeTabCollection
      │    │    ├── Tab4
      │    │    └── TreeTabNodeTabCollection
      │    │        └── Tab6
      ...
```


#### 2. TreeTabNode as Metadata Container  

**TreeTabNode** contains all tree-related metadata:

```cpp
class TreeTabNode {
  tree_tab::TreeTabNodeId id_;
  int level_ = 0;        // Depth in tree (root = 0)
  int height_ = 0;       // Height of subtree rooted at this node
  bool collapsed_ = false; // Hide/show child tabs
  
  // Tree structure calculations
  int CalculateLevelAndHeightRecursively();
  void OnChildHeightChanged();
};
```

#### 3. TreeTabModel as Registry

**TreeTabModel** only contains TreeTabNode references for access:

```cpp
class TreeTabModel {
  // Registry pointing to TreeTabNodes (doesn't own them)
  std::map<tree_tab::TreeTabNodeId, raw_ptr<const TreeTabNode>> tree_tab_nodes_;
  
  // Node lifecycle coordinated with TreeTabNodeTabCollection
  void AddTreeTabNode(const TreeTabNode& node);
  void RemoveTreeTabNode(const tree_tab::TreeTabNodeId& id);
  
  // Tree queries for UI components
  const TreeTabNode* GetNode(const tree_tab::TreeTabNodeId& id) const;
  int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const;
  
  // Callbacks for UI updates
  base::RepeatingCallbackList<void(const TreeTabNode&)> add_callbacks_;
  base::RepeatingCallbackList<void(const tree_tab::TreeTabNodeId&)> remove_callbacks_;
};
```

Note that this relationship is similar to the existing  **TabGroup** and 
**TabGroupTabCollection** relationship. Which means:
* **TreeTabNodeTabCollection** manages the lifecycle of tabs within the tree,
while the actual tree metadata is stored in a separate **TreeTabNode** class.  
* **TreeTabNode** is stored in **TreeTabModel** for UI access and it's owned by
  **TabStripModel**.
* Only **TabStripModel** can directly manipulate **TreeTabNodeTabCollection**.
* UI components access tree metadata through **TreeTabNode**.

**Following the Group Pattern**: So this mirrors how tab group classes are organized:

```cpp
// Group Pattern (existing):
TabGroupTabCollection owns TabGroup → TabGroupModel provides UI access

// Tree Pattern (proposed):
TreeTabNodeTabCollection owns TreeTabNode → TreeTabModel provides UI access
```

### Mode-Specific Behavior via Delegate Pattern
Currently, we have complicated conditionals scattered throughout the tab UI
regarding mode, such as:

```cpp
void Layout() {
  if (is_vertical_tab) {

  } else {

  }
}
```

So when we add tree tabs, we want to avoid adding more conditionals like:

```cpp
void Layout() {
  if (is_tree_tabs_mode) {
    // Tree-specific layout
  } else if (is_vertical_tab) {
    // Vertical tab layout
  } else {
    // Default layout
  }
  😭
}
```

This is really hard to maintain and extend. Instead, we use a delegate pattern
to cleanly separate mode-specific behavior while minimizing conditionals.

#### BraveTabStripCollectionDelegate - Base Delegate

```cpp
class BraveTabStripCollectionDelegate {
public:
  // Determines if this delegate should handle tab manipulation
  virtual bool ShouldHandleTabManipulation() const { return false; }
  
  // Pre/post-processing hooks for tab operations
  virtual void AddTabRecursive(std::unique_ptr<TabInterface> tab, size_t index,
                              std::optional<tab_groups::TabGroupId> new_group_id,
                              bool new_pinned_state,
                              TabInterface* opener) const {}
  
  virtual std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(size_t index) const { 
    return nullptr; 
  }
  
  virtual void MoveTabsRecursive(const std::vector<int>& tab_indices,
                                size_t destination_index,
                                /* ... */) const {}
};
```

**Purpose**: Provides a leeway for pre/post-processing of `TabStripCollection`'s behavior without modifying core collection logic.

#### BraveTreeTabStripCollectionDelegate - Tree Mode Specialization

```cpp
class BraveTreeTabStripCollectionDelegate : public BraveTabStripCollectionDelegate {
public:
  // Activates tree-specific handling
  bool ShouldHandleTabManipulation() const override { return true; }
  
  // Tree-aware tab operations with pre/post-processing
  void AddTabRecursive(std::unique_ptr<TabInterface> tab, size_t index,
                      std::optional<tab_groups::TabGroupId> new_group_id,
                      bool new_pinned_state,
                      TabInterface* opener) const override;
  std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(size_t index) const override;
  void MoveTabsRecursive(const std::vector<int>& tab_indices,
                        size_t destination_index,
                        /* ... */) const override;

private:
  base::WeakPtr<TreeTabModel> tree_tab_model_;
};
```

##### Clean Architecture Benefits

This delegate pattern eliminates the need for conditionals throughout the codebase:

```cpp
// Instead of this everywhere:
void TabStripCollection::AddTab(/*...*/) {
  if (is_tree_tabs_mode) {
    // Tree-specific logic
  } else {
    // Default logic  
  }
}

// We have clean delegation:
void BraveTabStripCollection::AddTabRecursive(/*...*/) {
  if (delegate_ && delegate_->ShouldHandleTabManipulation()) {
    delegate_->AddTabRecursive(/*...*/);  // Tree delegate handles it
  } else {
    // Default collection behavior
  }
}

// We can find tree-specific logic encapsulated in the delegate class only.
void BraveTreeTabStripCollectionDelegate::AddTabRecursive(/*...*/) {
  // Tree-specific addition logic here: Wrap a new tab in TreeTabNodeTabCollection.
}
```

### Observer Extensions

#### TreeTabChanged Notifications

The existing `TabStripModelObserver` is extended with tree-specific change notifications:

```cpp
struct TreeTabChange {
  enum Type { kNodeCreated, kNodeWillBeDestroyed };
  
  struct CreatedChange : public Delta {
    raw_ref<const TreeTabNode> node;
  };
  
  struct WillBeDestroyedChange : public Delta {  
    raw_ref<const TreeTabNode> node;
  };
};

class TabStripModelObserver {
  // Extended observer interface
  virtual void OnTreeTabChanged(const TreeTabChange& change);
};
```

**Following the TabGroupChange pattern**: This mirrors the existing **TabGroupChange notification system** mentioned earlier
and tree related changes also would be notified via BrowserTabStripController:

```cpp
// Existing pattern for groups:
void TabStripModel::NotifyTabGroupCreated(const tab_groups::TabGroupId& group) {
  TabGroupChange change(
      this, group,
      TabGroupChange::CreateChange(
          TabGroupChange::TabGroupCreationReason::kNewGroupCreated, nullptr));
  for (auto& observer : observers_) {
    observer.OnTabGroupChanged(change);
  }
}

// Proposed pattern for tree tabs:
void TabStripModel::NotifyTreeTabNodeCreated(const tabs::TreeTabNode& node) {
  auto change = TreeTabChange(node.id(), TreeTabChange::CreatedChange(node));
  for (auto& observer : observers_) {
    observer.OnTreeTabChanged(change);
  }
}
```

**Example: UI responding to tree changes**  
Similar to how **BrowserTabStripController** reacts to tab group changes, tree tab UI would be handled:

```cpp
void BraveBrowserTabStripController::OnTreeTabChanged(
    const TreeTabChange& change) {
  switch (change.type) {
    case TreeTabChange::Type::kNodeCreated: {
      ...
      tabstrip_->tab_at(index)->set_tree_tab_node(change.id);
      break;
    }
    case TreeTabChange::Type::kNodeWillBeDestroyed: {
      ...
      tabstrip_->tab_at(index)->set_tree_tab_node(std::nullopt);
      break;
    }
    ...
  }
}
```
