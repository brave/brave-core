/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_

#include <memory>
#include <optional>

#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"

namespace tabs {
class TreeTabNode;
}  // namespace tabs

namespace tree_tab {
class TreeTabNodeId;
}  // namespace tree_tab

class BraveBrowserTabStripController : public BrowserTabStripController {
 public:
  BraveBrowserTabStripController(TabStripModel* model,
                                 BrowserView* browser_view,
                                 std::unique_ptr<TabMenuModelFactory>
                                     menu_model_factory_override = nullptr);
  BraveBrowserTabStripController(const BraveBrowserTabStripController&) =
      delete;
  BraveBrowserTabStripController& operator=(
      const BraveBrowserTabStripController&) = delete;
  ~BraveBrowserTabStripController() override;

  Browser* browser() const { return browser_view_->browser(); }

  bool IsCommandEnabledForTab(TabStripModel::ContextMenuCommand command_id,
                              const Tab* tab);

  int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const;
  // Note that this can return nullptr if the tree tab node is not found.
  // There are some cases where TreeTabNodeId set in tab UI isn't cleared when
  // moving tabs to an group, because when the node is removed, the tab in model
  // is in detached state temporarily.
  const tabs::TreeTabNode* GetTreeTabNode(
      const tree_tab::TreeTabNodeId& id) const;
  void SetTreeTabNodeCollapsed(const tree_tab::TreeTabNodeId& id,
                               bool collapsed);
  bool IsInCollapsedTreeTabNode(const tree_tab::TreeTabNodeId& id) const;

  const tree_tab::TreeTabNodeId* GetClosestCollapsedAncestor(
      const tree_tab::TreeTabNodeId& id) const;

  // Returns the tree tab node id wrapping the given group, or nullptr if not
  // wrapped (e.g. tree tabs off). Only valid when model is BraveTabStripModel.
  const tree_tab::TreeTabNodeId* GetTreeTabNodeIdForGroup(
      tab_groups::TabGroupId group_id) const;

  // BrowserTabStripController overrides:
  void OnTreeTabChanged(const TreeTabChange& change) override;

  // BrowserTabStripController overrides:
  void ExecuteContextMenuCommand(int index,
                                 TabStripModel::ContextMenuCommand command_id,
                                 int event_flags) override;
  bool IsContextMenuCommandChecked(
      TabStripModel::ContextMenuCommand command_id) override;
  bool IsContextMenuCommandEnabled(
      int index,
      TabStripModel::ContextMenuCommand command_id) override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

 private:
  bool ShouldShowTreeTabs();

  void ExpandAllCollapsedAncestors(const tree_tab::TreeTabNodeId& id);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_
