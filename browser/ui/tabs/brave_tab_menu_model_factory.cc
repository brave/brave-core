/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model_factory.h"

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

namespace brave {

std::unique_ptr<ui::SimpleMenuModel> BraveTabMenuModelFactory::Create(
    ui::SimpleMenuModel::Delegate* delegate,
    TabMenuModelDelegate* tab_menu_model_delegate,
    TabStripModel* tab_strip,
    int index) {
  return std::make_unique<BraveTabMenuModel>(delegate, tab_menu_model_delegate,
                                             tab_strip, index);
}

}  // namespace brave
