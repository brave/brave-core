/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_BOOKMARK_CONTEXT_MENU_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_BOOKMARK_CONTEXT_MENU_CONTROLLER_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/toolbar/bookmark_bar_sub_menu_model.h"
#include "chrome/browser/ui/bookmarks/bookmark_context_menu_controller.h"
#include "ui/gfx/native_widget_types.h"

class Browser;
class Profile;
class PrefService;

namespace bookmarks {
class BookmarkModel;
}

class BraveBookmarkContextMenuController
    : public BookmarkContextMenuController {
 public:
  BraveBookmarkContextMenuController(
      gfx::NativeWindow parent_window,
      BookmarkContextMenuControllerDelegate* delegate,
      Browser* browser,
      Profile* profile,
      BookmarkLaunchLocation opened_from,
      const bookmarks::BookmarkNode* parent,
      const std::vector<raw_ptr<const bookmarks::BookmarkNode,
                                VectorExperimental>>& selection);

  BraveBookmarkContextMenuController(
      const BraveBookmarkContextMenuController&) = delete;
  BraveBookmarkContextMenuController& operator=(
      const BraveBookmarkContextMenuController&) = delete;

  ~BraveBookmarkContextMenuController() override;

  BookmarkBarSubMenuModel* GetBookmarkSubmenuModel();

  // ui::SimpleMenuModel::Delegate implementation:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

  std::u16string GetLabelForCommandId(int command_id) const override;

 private:
  friend class BraveBookmarkContextMenuTest;

  void AddBraveBookmarksSubmenu(Profile* profile);
  void AddShowAllBookmarksButtonMenu();

  void SetPrefsForTesting(PrefService* prefs);

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<bookmarks::BookmarkModel> bookmark_model_ = nullptr;
  std::unique_ptr<BookmarkBarSubMenuModel> brave_bookmarks_submenu_model_;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_BOOKMARK_CONTEXT_MENU_CONTROLLER_H_
