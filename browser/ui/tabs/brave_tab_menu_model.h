/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_

#include <memory>

#include "chrome/browser/ui/tabs/tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

class BraveTabMenuModel : public TabMenuModel {
 public:
  enum BraveTabContextMenuCommand {
    CommandStart = TabStripModel::CommandLast,
    CommandCloseOtherTabs,
    CommandRestoreTab,
    CommandBookmarkAllTabs,
    CommandLast,
  };

  BraveTabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                    TabStripModel* tab_strip_model,
                    int index);
  ~BraveTabMenuModel() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveTabMenuModelTest, Basics);

  void Build();

  int GetRestoreTabCommandStringId() const;

  int index_;
  TabStripModel* tab_strip_model_;
  std::unique_ptr<ui::SimpleMenuModel::Delegate> delegate_proxy_;

  DISALLOW_COPY_AND_ASSIGN(BraveTabMenuModel);
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
