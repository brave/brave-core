/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_system_menu_model_builder.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "ui/menus/simple_menu_model.h"

BraveSystemMenuModelBuilder::~BraveSystemMenuModelBuilder() = default;

void BraveSystemMenuModelBuilder::InsertBraveSystemMenuForBrowserWindow(
    ui::SimpleMenuModel* model) {
  std::optional<size_t> insert_after =
      model->GetIndexOfCommandId(IDC_BOOKMARK_ALL_TABS);

  auto get_next_position = [&]() -> std::optional<size_t> {
    if (insert_after) {
      insert_after.value() += 1;
    }
    return insert_after;
  };

  if (tabs::utils::SupportsBraveVerticalTabs(browser())) {
    if (auto pos = get_next_position()) {
      model->InsertCheckItemWithStringIdAt(pos.value(),
                                           IDC_TOGGLE_VERTICAL_TABS,
                                           IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);
    }
  }

  if (BrowserSupportsFocusMode(browser())) {
    if (auto pos = get_next_position()) {
      model->InsertCheckItemWithStringIdAt(pos.value(), IDC_TOGGLE_FOCUS_MODE,
                                           IDS_SYSTEM_MENU_FOCUS_MODE);
    }
  }
}

void BraveSystemMenuModelBuilder::BuildSystemMenuForBrowserWindow(
    ui::SimpleMenuModel* model) {
  SystemMenuModelBuilder::BuildSystemMenuForBrowserWindow(model);
  InsertBraveSystemMenuForBrowserWindow(model);
}
