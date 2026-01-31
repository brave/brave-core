/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/system_menu_model_delegate.h"

#include <string>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

#define IsCommandIdChecked IsCommandIdChecked_ChromiumImpl
#define GetLabelForCommandId GetLabelForCommandId_ChromiumImpl

#include <chrome/browser/ui/views/frame/system_menu_model_delegate.cc>

#undef GetLabelForCommandId
#undef IsCommandIdChecked

bool SystemMenuModelDelegate::IsCommandIdChecked(int command_id) const {
  if (command_id == IDC_TOGGLE_VERTICAL_TABS) {
    return tabs::utils::ShouldShowBraveVerticalTabs(browser_);
  }
  return IsCommandIdChecked_ChromiumImpl(command_id);
}

std::u16string SystemMenuModelDelegate::GetLabelForCommandId(
    int command_id) const {
  if (command_id == IDC_TOGGLE_VERTICAL_TABS) {
    // We're reusing upstream's |IDC_TOGGLE_VERTICAL_TABS| command id and it's
    // added to system menu from
    // BraveSystemMenuModelBuilder::InsertBraveSystemMenuForBrowserWindow(). As
    // upstream made this command as dynamic, its label is fetched from this
    // method. Upstream's GetLabelForCommandId() refers
    // tabs::VerticalTabStripStateController::From(browser_) to get label for
    // current vertical tab state. However,
    // tabs::VerticalTabStripStateController::From(browser_) is null now as
    // we're not using upstream's vertical tab implementation.
    if (!tabs::VerticalTabStripStateController::From(browser_)) {
      return l10n_util::GetStringUTF16(IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);
    }
  }
  return GetLabelForCommandId_ChromiumImpl(command_id);
}
