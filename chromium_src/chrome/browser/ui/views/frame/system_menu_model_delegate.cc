/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/system_menu_model_delegate.h"

#include <string>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#define IsCommandIdChecked IsCommandIdChecked_ChromiumImpl
#define GetLabelForCommandId GetLabelForCommandId_ChromiumImpl

#include <chrome/browser/ui/views/frame/system_menu_model_delegate.cc>

#undef GetLabelForCommandId
#undef IsCommandIdChecked

bool SystemMenuModelDelegate::IsCommandIdChecked(int command_id) const {
  if (command_id == IDC_TOGGLE_VERTICAL_TABS) {
    auto* vtc = VerticalTabController::From(browser_);
    return vtc && vtc->ShouldShowBraveVerticalTabs();
  }
  if (command_id == IDC_TOGGLE_FOCUS_MODE) {
    return IsFocusModeEnabled(browser_);
  }
  return IsCommandIdChecked_ChromiumImpl(command_id);
}

std::u16string SystemMenuModelDelegate::GetLabelForCommandId(
    int command_id) const {
  if (command_id == IDC_TOGGLE_VERTICAL_TABS) {
    // Upstream treats this command as dynamic and labels it "Show tabs
    // vertically". Brave's item is a check item for this window.
    auto* vtc = VerticalTabController::From(browser_);
    if (vtc && vtc->SupportsBraveVerticalTabs()) {
      return l10n_util::GetStringUTF16(IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);
    }
  }
  return GetLabelForCommandId_ChromiumImpl(command_id);
}
