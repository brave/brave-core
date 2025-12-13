/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/split_tab_menu_model.h"

#define GetCommandIdEnum GetCommandIdEnum_Chromium
#define GetCommandIdInt GetCommandIdInt_Chromium

// Avoid "kToggleLinkState not handled in switch" error.
#define BRAVE_SPLIT_TAB_MENU_MODEL_EXECUTE_COMMAND         \
  case CommandId::kToggleLinkState: {                      \
    split_tab_data->set_linked(!split_tab_data->linked()); \
    break;                                                 \
  }

#include <chrome/browser/ui/tabs/split_tab_menu_model.cc>

#undef BRAVE_SPLIT_TAB_MENU_MODEL_EXECUTE_COMMAND
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
