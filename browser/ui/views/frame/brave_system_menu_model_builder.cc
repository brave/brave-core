/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_system_menu_model_builder.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "ui/base/models/simple_menu_model.h"

BraveSystemMenuModelBuilder::BraveSystemMenuModelBuilder(
    ui::AcceleratorProvider* provider,
    Browser* browser)
    : SystemMenuModelBuilder(provider, browser) {}

BraveSystemMenuModelBuilder::~BraveSystemMenuModelBuilder() = default;

void BraveSystemMenuModelBuilder::InsertBraveSystemMenuForBrowserWindow(
    ui::SimpleMenuModel* model) {
  if (tabs::utils::SupportsVerticalTabs(browser())) {
    std::optional<size_t> task_manager_index =
        model->GetIndexOfCommandId(IDC_TASK_MANAGER);

    if (task_manager_index.has_value()) {
      model->InsertCheckItemWithStringIdAt(task_manager_index.value(),
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
