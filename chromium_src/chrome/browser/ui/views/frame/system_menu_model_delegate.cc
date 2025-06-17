/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/system_menu_model_delegate.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

#define IsCommandIdChecked IsCommandIdChecked_ChromiumImpl
#include <chrome/browser/ui/views/frame/system_menu_model_delegate.cc>
#undef IsCommandIdChecked

bool SystemMenuModelDelegate::IsCommandIdChecked(int command_id) const {
  if (command_id == IDC_TOGGLE_VERTICAL_TABS) {
    return tabs::utils::ShouldShowVerticalTabs(browser_);
  }
  return IsCommandIdChecked_ChromiumImpl(command_id);
}
