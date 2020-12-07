/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_

#include <vector>

#include "chrome/browser/ui/tabs/tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace content {
class WebContents;
}  // namespace content

namespace sessions {
class TabRestoreService;
}  // namespace sessions

class BraveTabMenuModel : public TabMenuModel {
 public:
  enum BraveTabContextMenuCommand {
    CommandStart = TabStripModel::CommandLast,
    CommandRestoreTab,
    CommandBookmarkAllTabs,
    CommandMuteTab,
    CommandLast,
  };

  BraveTabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                    TabStripModel* tab_strip_model,
                    int index);
  ~BraveTabMenuModel() override;

  bool AreAllTabsMuted(const TabStripModel& tab_strip_model,
                       const std::vector<int>& indices) const;

 private:
  void Build(TabStripModel* tab_strip_model, int index);
  int GetRestoreTabCommandStringId() const;

  content::WebContents* web_contents_;
  sessions::TabRestoreService* restore_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveTabMenuModel);
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_MENU_MODEL_H_
