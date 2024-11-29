/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/bookmarks/brave_bookmark_context_menu.h"

#include <vector>

#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_context_menu.h"
#include "chrome/grit/generated_resources.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"

BraveBookmarkContextMenu::BraveBookmarkContextMenu(
    views::Widget* parent_widget,
    Browser* browser,
    Profile* profile,
    BookmarkLaunchLocation opened_from,
    const std::vector<
        raw_ptr<const bookmarks::BookmarkNode, VectorExperimental>>& selection,
    bool close_on_remove)
    : BookmarkContextMenu(parent_widget,
                          browser,
                          profile,
                          opened_from,
                          selection,
                          close_on_remove) {
  auto* submenu = menu()->GetMenuItemByID(IDC_BRAVE_BOOKMARK_BAR_SUBMENU);
  DCHECK(submenu);
  auto* submenu_model = controller_->GetBookmarkSubmenuModel();
  for (size_t j = 0; j < submenu_model->GetItemCount(); ++j) {
    views::MenuModelAdapter::AppendMenuItemFromModel(
        submenu_model, j, submenu, submenu_model->GetCommandIdAt(j));
  }
}

BraveBookmarkContextMenu::~BraveBookmarkContextMenu() = default;

BraveBookmarkContextMenuController*
BraveBookmarkContextMenu::GetControllerForTesting() {
  return static_cast<BraveBookmarkContextMenuController*>(
      controller_.get());  // IN-TEST
}
