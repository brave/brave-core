/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_BOOKMARK_SUB_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_BOOKMARK_SUB_MENU_MODEL_H_

#include <memory>

#include "brave/browser/ui/toolbar/bookmark_bar_sub_menu_model.h"
#include "chrome/browser/ui/toolbar/bookmark_sub_menu_model.h"

class Browser;

class BraveBookmarkSubMenuModel : public BookmarkSubMenuModel {
 public:
  BraveBookmarkSubMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                            Browser* browser);

  BraveBookmarkSubMenuModel(const BraveBookmarkSubMenuModel&) = delete;
  BraveBookmarkSubMenuModel& operator=(const BraveBookmarkSubMenuModel&) =
      delete;

  ~BraveBookmarkSubMenuModel() override;

 private:
  void Build(Browser* browser);

  std::unique_ptr<BookmarkBarSubMenuModel> brave_bookmarks_submenu_model_;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_BOOKMARK_SUB_MENU_MODEL_H_
