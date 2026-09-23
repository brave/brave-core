/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/split_tab_menu_model.h"

#include <chrome/browser/ui/tabs/split_tab_menu_model.cc>

// static
SplitTabMenuModel::CommandId SplitTabMenuModel::GetCommandIdEnum(
    int command_id) {
  return GetCommandIdEnum_ChromiumImpl(command_id);
}

// static
int SplitTabMenuModel::GetCommandIdInt(
    SplitTabMenuModel::CommandId command_id) {
  return GetCommandIdInt_ChromiumImpl(command_id);
}
