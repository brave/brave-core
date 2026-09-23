/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bookmarks/bookmark_context_menu.h"

#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"

// Upstream's `BookmarkContextMenu` constructor only descends into the submenu
// whose command id is `IDC_BOOKMARK_BAR_SUBMENU`. But, we also have the
// containers submenu `IDC_OPEN_IN_CONTAINER` that needs to be populated, so it
// seems better to patch here to populate our submenu.
#define BRAVE_BOOKMARK_CONTEXT_MENU                                            \
  if (menu_model->GetCommandIdAt(i) == IDC_OPEN_IN_CONTAINER) {                \
    views::MenuItemView* item = menu_->GetMenuItemByID(IDC_OPEN_IN_CONTAINER); \
    ui::MenuModel* container_submodel = menu_model->GetSubmenuModelAt(i);      \
    DCHECK(container_submodel);                                                \
    for (size_t j = 0; j < container_submodel->GetItemCount(); ++j) {          \
      views::MenuModelAdapter::AppendMenuItemFromModel(                        \
          container_submodel, j, item, container_submodel->GetCommandIdAt(j)); \
    }                                                                          \
  }

#define BookmarkContextMenuController BraveBookmarkContextMenuController
#include <chrome/browser/ui/views/bookmarks/bookmark_context_menu.cc>
#undef BookmarkContextMenuController
#undef BRAVE_BOOKMARK_CONTEXT_MENU
