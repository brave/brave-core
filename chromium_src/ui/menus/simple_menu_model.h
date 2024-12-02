/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_MENUS_SIMPLE_MENU_MODEL_H_
#define BRAVE_CHROMIUM_SRC_UI_MENUS_SIMPLE_MENU_MODEL_H_

#define AddButtonItem                                                        \
  AddButtonItemAt(int command_id, ButtonMenuItemModel* model, size_t index); \
  void AddButtonItem

#include "src/ui/menus/simple_menu_model.h"  // IWYU pragma: export

#undef AddButtonItem

#endif  // BRAVE_CHROMIUM_SRC_UI_MENUS_SIMPLE_MENU_MODEL_H_
