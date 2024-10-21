/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/ui/menus/simple_menu_model.cc"

namespace ui {

void SimpleMenuModel::AddButtonItemAt(int command_id,
                                      ButtonMenuItemModel* model,
                                      size_t index) {
  Item item(command_id, TYPE_BUTTON_ITEM, std::u16string());
  item.button_model = model;
  InsertItemAtIndex(std::move(item), index);
}

}  // namespace ui
