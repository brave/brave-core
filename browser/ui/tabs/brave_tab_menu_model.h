/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_

#include <vector>

#include "chrome/browser/ui/tabs/tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

class Browser;
class TabStripModel;

namespace sessions {
class TabRestoreService;
}  // namespace sessions

// This proxies TabContextMenuContents and handles brave's commands and
// pass other chromium's commands to TabContextMenuContents(delegate_).
class BraveTabMenuModel : public TabMenuModel,
                          public ui::SimpleMenuModel::Delegate {
 public:
  BraveTabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                    TabStripModel* tab_strip_model,
                    int index);
  ~BraveTabMenuModel() override;

  // Overridden from ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveTabMenuModelTest, Basics);

  enum BraveTabContextMenuCommand {
    CommandStart = TabStripModel::CommandLast,
    CommandCloseOtherTabs,
    CommandRestoreTab,
    CommandBookmarkAllTabs,
    CommandLast,
  };

  bool IsBraveCommandIdEnabled(int command_id) const;
  void ExecuteBraveCommand(int command_id);
  std::vector<int> GetIndicesToClosed(int index) const;
  bool IsBraveCommandId(int command_id) const;

  void Build();

  int GetRestoreTabCommandStringId() const;

  int index_;
  TabStripModel* tab_strip_model_;
  ui::SimpleMenuModel::Delegate* delegate_;
  Browser* browser_ = nullptr;
  sessions::TabRestoreService* restore_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveTabMenuModel);
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
