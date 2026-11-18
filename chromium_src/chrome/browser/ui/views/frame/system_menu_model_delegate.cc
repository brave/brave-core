/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/system_menu_model_delegate.h"

#include <optional>
#include <string>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"

namespace {

std::optional<bool> MaybeHandleBraveCommand(int command_id,
                                            BrowserWindowInterface* browser) {
  if (command_id == IDC_TOGGLE_VERTICAL_TABS) {
    auto* vtc = VerticalTabController::From(browser);
    return vtc && vtc->ShouldShowBraveVerticalTabs();
  }
  if (command_id == IDC_TOGGLE_FOCUS_MODE) {
    return IsFocusModeEnabled(browser);
  }

  return std::nullopt;
}

std::optional<std::u16string> MaybeGetLabelForBraveCommand(
    int command_id,
    BrowserWindowInterface* browser) {
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
    if (!tabs::VerticalTabStripStateController::From(browser)) {
      return l10n_util::GetStringUTF16(IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);
    }
  }

  return std::nullopt;
}

}  // namespace

#include <chrome/browser/ui/views/frame/system_menu_model_delegate.cc>
