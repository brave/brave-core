/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BOOKMARK_BAR_SUB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BOOKMARK_BAR_SUB_MENU_MODEL_H_

#include "base/memory/raw_ptr.h"
#include "ui/menus/simple_menu_model.h"

class Profile;

class BookmarkBarSubMenuModel : public ui::SimpleMenuModel,
                                public ui::SimpleMenuModel::Delegate {
 public:
  explicit BookmarkBarSubMenuModel(Profile* profile);

  BookmarkBarSubMenuModel(const BookmarkBarSubMenuModel&) = delete;
  BookmarkBarSubMenuModel& operator=(const BookmarkBarSubMenuModel&) = delete;

  ~BookmarkBarSubMenuModel() override;

  // ui::SimpleMenuModel::Delegate
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;

 private:
  void Build();

  raw_ptr<Profile> profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BOOKMARK_BAR_SUB_MENU_MODEL_H_
