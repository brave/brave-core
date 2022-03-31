/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_app_menu.h"

#include <memory>

#include "brave/app/brave_command_ids.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/menu/menu_item_view.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_status_label.h"
#include "brave/browser/ui/views/toolbar/brave_vpn_toggle_button.h"
#endif

using views::MenuItemView;

BraveAppMenu::~BraveAppMenu() = default;

MenuItemView* BraveAppMenu::AddMenuItem(views::MenuItemView* parent,
                                        int menu_index,
                                        ui::MenuModel* model,
                                        int model_index,
                                        ui::MenuModel::ItemType menu_type) {
  MenuItemView* menu_item =
      AppMenu::AddMenuItem(parent, menu_index, model, model_index, menu_type);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (menu_item && model->GetCommandIdAt(model_index) == IDC_TOGGLE_BRAVE_VPN) {
    menu_item->AddChildView(std::make_unique<BraveVPNStatusLabel>(browser_));
    menu_item->AddChildView(std::make_unique<BraveVPNToggleButton>(browser_));
  }
#endif

  return menu_item;
}
