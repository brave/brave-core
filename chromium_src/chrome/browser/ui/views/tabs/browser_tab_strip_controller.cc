/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"

#include <memory>

#include "chrome/browser/ui/tabs/tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_menu_model_delegate.h"

#include <chrome/browser/ui/views/tabs/browser_tab_strip_controller.cc>

std::unique_ptr<TabMenuModel> BrowserTabStripController::CreateTabMenuModel(
    ui::SimpleMenuModel::Delegate* delegate,
    TabMenuModelDelegate* tab_menu_model_delegate,
    TabStripModel* tab_strip_model,
    tabs::TabHandle context_tab) {
  return std::make_unique<TabMenuModel>(delegate, tab_menu_model_delegate,
                                        tab_strip_model, context_tab);
}
