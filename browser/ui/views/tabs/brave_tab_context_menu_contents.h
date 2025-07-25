/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTEXT_MENU_CONTENTS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTEXT_MENU_CONTENTS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "ui/base/mojom/menu_source_type.mojom.h"
#include "ui/base/ui_base_types.h"
#include "ui/menus/simple_menu_model.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/ui/containers/containers_menu_model.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

class BraveBrowserTabStripController;
class BraveTabMenuModel;
class Browser;
class Tab;

namespace gfx {
class Point;
}  // namespace gfx

namespace sessions {
class TabRestoreService;
}  // namespace sessions

namespace views {
class MenuRunner;
}  // namespace views

class BraveTabContextMenuContents
    : public ui::SimpleMenuModel::Delegate
#if BUILDFLAG(ENABLE_CONTAINERS)
    ,
      public containers::ContainersMenuModel::Delegate
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
{
 public:
  BraveTabContextMenuContents(Tab* tab,
                              BraveBrowserTabStripController* controller,
                              int index);
  BraveTabContextMenuContents(const BraveTabContextMenuContents&) = delete;
  BraveTabContextMenuContents& operator=(const BraveTabContextMenuContents&) =
      delete;
  ~BraveTabContextMenuContents() override;

  void Cancel();

  void RunMenuAt(const gfx::Point& point,
                 ui::mojom::MenuSourceType source_type);

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

#if BUILDFLAG(ENABLE_CONTAINERS)
  // ContainersMenuModel::Delegate:
  void OnContainerSelected(
      const containers::mojom::ContainerPtr& container) override;
  base::flat_set<std::string> GetCurrentContainerIds() override;
  Browser* GetBrowserToOpenSettings() override;
  float GetScaleFactor() override;
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

 private:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripStringBrowserTest,
                           ContextMenuString);

  bool IsBraveCommandIdEnabled(int command_id) const;
  void ExecuteBraveCommand(int command_id);
  void BringAllTabsToThisWindow();

  bool IsBraveCommandId(int command_id) const;
  bool IsValidContextMenu() const;
  void OnMenuClosed();

  void NewSplitView();
  void TileSelectedTabs();
  void BreakSelectedTile();
  void SwapTabsInTile();

  std::vector<int> GetTabIndicesForSplitViewCommand() const;

  std::unique_ptr<BraveTabMenuModel> model_;
  std::unique_ptr<views::MenuRunner> menu_runner_;

  raw_ptr<Tab> tab_ = nullptr;
  int tab_index_ = -1;

  // true when menu is closed.
  // If it's set to true, this instance will not be used anymore because
  // new instance is created when showing context menu.
  bool menu_closed_ = false;
  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<sessions::TabRestoreService> restore_service_ = nullptr;
  raw_ptr<BraveBrowserTabStripController> controller_ = nullptr;

  base::WeakPtrFactory<BraveTabContextMenuContents> weak_ptr_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTEXT_MENU_CONTENTS_H_
