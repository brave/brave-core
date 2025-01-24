/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"

#include <memory>
#include <vector>

#include "base/check_is_test.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/bookmark/brave_bookmark_prefs.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/bookmarks/bookmark_context_menu_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

BraveBookmarkContextMenuController::BraveBookmarkContextMenuController(
    gfx::NativeWindow parent_window,
    BookmarkContextMenuControllerDelegate* delegate,
    Browser* browser,
    Profile* profile,
    BookmarkLaunchLocation opened_from,
    const std::vector<
        raw_ptr<const bookmarks::BookmarkNode, VectorExperimental>>& selection)
    : BookmarkContextMenuController(parent_window,
                                    delegate,
                                    browser,
                                    profile,
                                    opened_from,
                                    selection),
      browser_(browser),
      prefs_(browser_ ? browser_->profile()->GetPrefs() : nullptr) {
  if (!browser_) {
    CHECK_IS_TEST();
  }
  AddBraveBookmarksSubmenu(profile);
  AddShowAllBookmarksButtonMenu();
  bookmark_model_ = BookmarkModelFactory::GetForBrowserContext(profile);
}

BraveBookmarkContextMenuController::~BraveBookmarkContextMenuController() =
    default;

void BraveBookmarkContextMenuController::AddBraveBookmarksSubmenu(
    Profile* profile) {
  auto index = menu_model()->GetIndexOfCommandId(IDC_BOOKMARK_BAR_ALWAYS_SHOW);
  if (!index.has_value()) {
    return;
  }
  menu_model()->RemoveItemAt(index.value());
  brave_bookmarks_submenu_model_ =
      std::make_unique<BookmarkBarSubMenuModel>(profile);

  menu_model()->InsertSubMenuWithStringIdAt(
      index.value(), IDC_BRAVE_BOOKMARK_BAR_SUBMENU, IDS_SHOW_BOOKMARK_BAR,
      brave_bookmarks_submenu_model_.get());
}

bool BraveBookmarkContextMenuController::IsCommandIdChecked(
    int command_id) const {
  if (brave_bookmarks_submenu_model_->GetIndexOfCommandId(command_id)) {
    return brave_bookmarks_submenu_model_->IsCommandIdChecked(command_id);
  }

  if (command_id == IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY) {
    // Even test sets prefs for testing, there could be timing when prefs_ is
    // nullptr on creation.
    if (!prefs_) {
      CHECK_IS_TEST();
      return false;
    }
    return prefs_->GetBoolean(brave::bookmarks::prefs::kShowAllBookmarksButton);
  }

  return BookmarkContextMenuController::IsCommandIdChecked(command_id);
}

bool BraveBookmarkContextMenuController::IsCommandIdEnabled(
    int command_id) const {
  if (brave_bookmarks_submenu_model_->GetIndexOfCommandId(command_id)) {
    return brave_bookmarks_submenu_model_->IsCommandIdEnabled(command_id);
  }

  if (command_id == IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY) {
    return true;
  }

  return BookmarkContextMenuController::IsCommandIdEnabled(command_id);
}

bool BraveBookmarkContextMenuController::IsCommandIdVisible(
    int command_id) const {
  if (brave_bookmarks_submenu_model_->GetIndexOfCommandId(command_id)) {
    return brave_bookmarks_submenu_model_->IsCommandIdVisible(command_id);
  }

  if (command_id == IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY) {
    // If the 'Other Bookmarks' node has no children, then hiding the 'Show all
    // bookmarks button' option from drop down as showing the option and the
    // 'All Bookmarks' button serves no purpose.
    // returning false if other bookmark node is empty, else true.
    return !bookmark_model_->other_node()->children().empty();
  }

  return BookmarkContextMenuController::IsCommandIdVisible(command_id);
}

void BraveBookmarkContextMenuController::ExecuteCommand(int command_id,
                                                        int event_flags) {
  if (brave_bookmarks_submenu_model_->GetIndexOfCommandId(command_id)) {
    brave_bookmarks_submenu_model_->ExecuteCommand(command_id, event_flags);
    return;
  }

  if (command_id == IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY) {
    if (!browser_) {
      CHECK_IS_TEST();
    }

    brave::ToggleAllBookmarksButtonVisibility(browser_);
    return;
  }

  BookmarkContextMenuController::ExecuteCommand(command_id, event_flags);
}

std::u16string BraveBookmarkContextMenuController::GetLabelForCommandId(
    int command_id) const {
  if (brave_bookmarks_submenu_model_->GetIndexOfCommandId(command_id)) {
    return brave_bookmarks_submenu_model_->GetLabelForCommandId(command_id);
  }

  if (command_id == IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY) {
    return l10n_util::GetStringUTF16(IDS_SHOW_ALL_BOOKMARKS_BUTTON);
  }

  return BookmarkContextMenuController::GetLabelForCommandId(command_id);
}

BookmarkBarSubMenuModel*
BraveBookmarkContextMenuController::GetBookmarkSubmenuModel() {
  return brave_bookmarks_submenu_model_.get();
}

void BraveBookmarkContextMenuController::AddShowAllBookmarksButtonMenu() {
  menu_model()->AddCheckItemWithStringId(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY,
      IDS_SHOW_ALL_BOOKMARKS_BUTTON);
}

void BraveBookmarkContextMenuController::SetPrefsForTesting(
    PrefService* prefs) {
  prefs_ = prefs;  // IN-TEST
}
