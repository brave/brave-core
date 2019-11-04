/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_TAB_MENU_MODEL_DELEGATE_PROXY_H_
#define BRAVE_BROWSER_UI_TABS_TAB_MENU_MODEL_DELEGATE_PROXY_H_

#include <vector>

#include "ui/base/models/simple_menu_model.h"

class Browser;
class TabStripModel;

namespace sessions {
class TabRestoreService;
}  // namespace sessions

// This proxies |TabMenuModelDelegate| and it handles brave's commands instead
// of sending it to |TabMenuModelDelegate|.
class TabMenuModelDelegateProxy : public ui::SimpleMenuModel::Delegate {
 public:
  TabMenuModelDelegateProxy(ui::SimpleMenuModel::Delegate* delegate,
                            TabStripModel* tab_strip_model,
                            int index);
  ~TabMenuModelDelegateProxy() override;

  // Overridden from ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override;

  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  bool IsBraveCommandIdEnabled(int command_id) const;
  void ExecuteBraveCommand(int command_id);
  std::vector<int> GetIndicesClosed(int index) const;
  bool IsBraveCommandIds(int command_id) const;

  ui::SimpleMenuModel::Delegate* delegate_;
  TabStripModel* tab_strip_model_;
  int index_;
  Browser* browser_ = nullptr;
  sessions::TabRestoreService* restore_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TabMenuModelDelegateProxy);
};

#endif  // BRAVE_BROWSER_UI_TABS_TAB_MENU_MODEL_DELEGATE_PROXY_H_

