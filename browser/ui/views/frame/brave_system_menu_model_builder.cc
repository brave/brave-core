/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_system_menu_model_builder.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "ui/menus/simple_menu_model.h"

BraveSystemMenuModelBuilder::~BraveSystemMenuModelBuilder() = default;

void BraveSystemMenuModelBuilder::InsertBraveSystemMenuForBrowserWindow(
    ui::SimpleMenuModel* model) {
  if (tabs::utils::SupportsBraveVerticalTabs(browser())) {
    std::optional<size_t> bookmark_all_tabs_index =
        model->GetIndexOfCommandId(IDC_BOOKMARK_ALL_TABS);

    if (bookmark_all_tabs_index.has_value()) {
      model->InsertCheckItemWithStringIdAt(bookmark_all_tabs_index.value() + 1,
                                           IDC_TOGGLE_VERTICAL_TABS,
                                           IDS_TAB_CXMENU_SHOW_VERTICAL_TABS);
    }
  }
}

void BraveSystemMenuModelBuilder::BuildSystemMenuForBrowserWindow(
    ui::SimpleMenuModel* model) {
  SystemMenuModelBuilder::BuildSystemMenuForBrowserWindow(model);
  InsertBraveSystemMenuForBrowserWindow(model);
}
