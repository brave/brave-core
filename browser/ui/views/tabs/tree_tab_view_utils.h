/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_TREE_TAB_VIEW_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_TREE_TAB_VIEW_UTILS_H_

#include <functional>
#include <string>
#include <vector>

#include "components/tabs/public/tab_collection_types.h"

// Hooks for showing tree tabs in Chromium's tab strip views
// (TabCollectionNode based, used by Chromium's vertical tabs). The chromium_src
// overrides of those views forward-declare these instead of including this
// header, so keep the signatures in sync with them.
//
// Tree tab node collections are unwrapped in the view model: their children
// take their place, so that the view hierarchy keeps the shape Chromium's views
// expect, i.e. UNPINNED contains tabs, groups and splits only, and groups and
// splits are as they are. The tree hierarchy is then expressed by indentation.
// TabCollectionNode and RootTabCollectionNode don't know about tree tabs: they
// read the hierarchy through GetUnwrappedChildren(), and
// BraveTabStripCollection notifies them of the changes of the unwrapped
// hierarchy.
namespace tree_tab_view_utils {

// Returns the children of |collection| with tree tab node collections
// unwrapped, recursively. The result is in tab strip model order.
std::vector<std::reference_wrapper<const tabs::Child>> GetUnwrappedChildren(
    const tabs::TabCollection& collection);

#if 0
// Returns the parent of |item| (a tab or a collection) with tree tab node
// collections unwrapped, i.e. the nearest ancestor that isn't a tree tab node
// collection. Returns nullptr if |item| isn't in a hierarchy.
const tabs::TabCollection* GetUnwrappedParent(const tabs::ConstChildPtr& item);

// Returns the item that comes right before |item| among the unwrapped children
// of its unwrapped parent, or nullopt if |item| is the first one.
std::optional<tabs::ConstChildPtr> GetUnwrappedPredecessor(
    const tabs::ConstChildPtr& item);

// Returns true if |item| is in the hierarchy under |root|.
bool IsInTabStrip(const tabs::TabCollection* root,
                  const tabs::ConstChildPtr& item);

// Returns the tabs of the tree tab node that |change| is about.
std::vector<const tabs::TabInterface*> GetTabsOfTreeTabChange(
    TabStripModel* model,
    const TreeTabChange& change);

// Returns the indentation for |view| (a TabView, TabGroupView or SplitTabView)
// based on its tree tab level. |available_width| is the width |view| would
// have without indentation.
int GetTreeTabIndent(const views::View* view, int available_width);

// Returns true if |view| (a TabView, TabGroupView or SplitTabView) is hidden
// because one of its tree tab ancestors is collapsed.
bool IsInCollapsedTreeTabNode(const views::View* view);
#endif

// Returns true if |tab| is directly wrapped by a tree tab node that has
// descendants.
bool HasTreeTabDescendants(const tabs::TabInterface* tab);

// Returns true if |tab| is directly wrapped by a collapsed tree tab node.
bool IsTreeTabNodeCollapsed(const tabs::TabInterface* tab);

// Returns the tooltip and accessible name of the toggle button that collapses
// or expands |tab|'s tree tab node.
std::u16string GetTreeTabToggleButtonLabel(const tabs::TabInterface* tab);

// Collapses or expands the tree tab node directly wrapping |tab|.
void ToggleTreeTabNodeCollapsed(tabs::TabInterface* tab);

}  // namespace tree_tab_view_utils

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_TREE_TAB_VIEW_UTILS_H_
