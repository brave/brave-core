/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bookmarks/bookmark_context_menu.h"

#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"

// Upstream's `BookmarkContextMenu` constructor only descends into the submenu
// whose command id is `IDC_BOOKMARK_BAR_SUBMENU` which is only created when
// `ntp_features::kNtpSimplificationBookmarkBar` is enabled (disabled by
// default for now). We could just remap the id with our
// `IDC_BRAVE_BOOKMARK_BAR_SUBMENU`, so the upstream populates Brave's submenu,
// but we also have the containers submenu `DC_OPEN_IN_CONTAINER` that needs to
// be populated, so it seems better to patch here to populate all our submenus.
#define BRAVE_BOOKMARK_CONTEXT_MENU                                    \
  if (menu_model->GetTypeAt(i) == ui::MenuModel::TYPE_SUBMENU) {       \
    views::MenuItemView* item =                                        \
        menu_->GetMenuItemByID(menu_model->GetCommandIdAt(i));         \
    ui::MenuModel* brave_submodel = menu_model->GetSubmenuModelAt(i);  \
    DCHECK(brave_submodel);                                            \
    for (size_t j = 0; j < brave_submodel->GetItemCount(); ++j) {      \
      views::MenuModelAdapter::AppendMenuItemFromModel(                \
          brave_submodel, j, item, brave_submodel->GetCommandIdAt(j)); \
    }                                                                  \
  }

#define BookmarkContextMenuController BraveBookmarkContextMenuController
#include <chrome/browser/ui/views/bookmarks/bookmark_context_menu.cc>
#undef BookmarkContextMenuController
#undef BRAVE_BOOKMARK_CONTEXT_MENU
