/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/bookmarks/brave_bookmark_context_menu.h"

#include <vector>

#include "base/check.h"
#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_context_menu.h"
#include "chrome/grit/generated_resources.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/app/brave_command_ids.h"
#endif

BraveBookmarkContextMenu::BraveBookmarkContextMenu(
    views::Widget* parent_widget,
    Browser* browser,
    Profile* profile,
    BookmarkLaunchLocation opened_from,
    const std::vector<
        raw_ptr<const bookmarks::BookmarkNode, VectorExperimental>>& selection,
    bool close_on_remove,
    bool can_paste)
    : BookmarkContextMenu(parent_widget,
                          browser,
                          profile,
                          opened_from,
                          selection,
                          close_on_remove,
                          can_paste) {
  auto* submenu = menu()->GetMenuItemByID(IDC_BRAVE_BOOKMARK_BAR_SUBMENU);
  DCHECK(submenu);
  auto* submenu_model = controller_->GetBookmarkSubmenuModel();
  for (size_t j = 0; j < submenu_model->GetItemCount(); ++j) {
    views::MenuModelAdapter::AppendMenuItemFromModel(
        submenu_model, j, submenu, submenu_model->GetCommandIdAt(j));
  }

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (auto* containers_model =
          controller_->GetContainersBookmarkSubmenuModel()) {
    if (auto* containers_menu =
            menu()->GetMenuItemByID(IDC_OPEN_IN_CONTAINER)) {
      for (size_t j = 0; j < containers_model->GetItemCount(); ++j) {
        views::MenuModelAdapter::AppendMenuItemFromModel(
            containers_model, j, containers_menu,
            containers_model->GetCommandIdAt(j));
      }
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

BraveBookmarkContextMenu::~BraveBookmarkContextMenu() = default;

BraveBookmarkContextMenuController*
BraveBookmarkContextMenu::GetControllerForTesting() {
  return static_cast<BraveBookmarkContextMenuController*>(
      controller_.get());  // IN-TEST
}
