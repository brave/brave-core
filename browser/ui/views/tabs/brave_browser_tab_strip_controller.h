/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_

#include <memory>
#include <optional>

#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"

class BraveTabContextMenuContents;

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

  const std::optional<int> GetModelIndexOf(Tab* tab);

  // Enters rename mode for the tab at the given index. This only affects UI
  // side.
  void EnterTabRenameModeAt(int index);

  // Sets the custom title for the tab at the specified index.
  void SetCustomTitleForTab(int index,
                            const std::optional<std::u16string>& title);

  // Gets the TreeTabNode for the given id.
  const tabs::TreeTabNode& GetTreeTabNode(
      const tree_tab::TreeTabNodeId& id) const;

  // Returns the height of the tree that given TreeTabNodeId belongs to.
  int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const;

  // BrowserTabStripController overrides:
  void ShowContextMenuForTab(Tab* tab,
                             const gfx::Point& p,
                             ui::mojom::MenuSourceType source_type) override;
  void ExecuteCommandForTab(TabStripModel::ContextMenuCommand command_id,
                            const Tab* tab) override;
  void OnTreeTabChanged(const TreeTabChange& change) override;

 private:
  // If non-NULL it means we're showing a menu for the tab.
  std::unique_ptr<BraveTabContextMenuContents> context_menu_contents_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_BROWSER_TAB_STRIP_CONTROLLER_H_
