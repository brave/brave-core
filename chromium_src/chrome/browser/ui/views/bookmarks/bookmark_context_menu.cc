/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bookmarks/bookmark_context_menu.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"

namespace {

void MaybeAppendContainerSubmenu(int i,
                                 ui::MenuModel* menu_model,
                                 views::MenuItemView* menu) {
  if (menu_model->GetCommandIdAt(i) == IDC_OPEN_IN_CONTAINER) {
    views::MenuItemView* item = menu->GetMenuItemByID(IDC_OPEN_IN_CONTAINER);
    ui::MenuModel* submenu_model = menu_model->GetSubmenuModelAt(i);
    DCHECK(submenu_model);
    for (size_t j = 0; j < submenu_model->GetItemCount(); ++j) {
      views::MenuModelAdapter::AppendMenuItemFromModel(
          submenu_model, j, item, submenu_model->GetCommandIdAt(j));
    }
  }
}

}  // namespace

#include <chrome/browser/ui/views/bookmarks/bookmark_context_menu.cc>
