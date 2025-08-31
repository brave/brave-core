/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/split_tab_menu_model.h"

#define GetCommandIdEnum GetCommandIdEnum_Chromium
#define GetCommandIdInt GetCommandIdInt_Chromium

#include <chrome/browser/ui/tabs/split_tab_menu_model.cc>

#undef GetCommandIdInt
#undef GetCommandIdEnum

// static
SplitTabMenuModel::CommandId SplitTabMenuModel::GetCommandIdEnum(
    int command_id) {
  return GetCommandIdEnum_Chromium(command_id);
}

// static
int SplitTabMenuModel::GetCommandIdInt(
    SplitTabMenuModel::CommandId command_id) {
  return GetCommandIdInt_Chromium(command_id);
}
