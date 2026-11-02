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

namespace views {
class View;
}  // namespace views

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

// Returns the indentation for |view| (a TabView, TabGroupView or SplitTabView)
// based on its tree tab level. |available_width| is the width |view| would
// have without indentation.
int GetTreeTabIndent(const views::View* view, int available_width);

// Returns true if |view| (a TabView, TabGroupView or SplitTabView) is hidden
// because one of its tree tab ancestors is collapsed.
bool IsInCollapsedTreeTabNode(const views::View* view);

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

// Adds the tree tab descendants of |tab| to the selection if |tab| is
// selected.
void AddTreeTabDescendantsToSelection(tabs::TabInterface* tab);

// Expands the collapsed tree tab ancestors of |tab| so that it's visible.
void ExpandCollapsedTreeTabAncestors(const tabs::TabInterface* tab);

}  // namespace tree_tab_view_utils

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_TREE_TAB_VIEW_UTILS_H_
