/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_vpn_menu_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_commands.h"

BraveVPNMenuModel::BraveVPNMenuModel(Browser* browser)
    : SimpleMenuModel(nullptr), browser_(browser) {
  set_delegate(this);
  Build();
}

BraveVPNMenuModel::~BraveVPNMenuModel() = default;

void BraveVPNMenuModel::Build() {
  AddToggleItemWithStringId(IDC_TOGGLE_BRAVE_VPN, IDS_BRAVE_VPN_MENU);
  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItemWithStringId(IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON,
                      IDS_BRAVE_VPN_SHOW_VPN_BUTTON_MENU_ITEM);
  AddItemWithStringId(IDC_SEND_BRAVE_VPN_FEEDBACK,
                      IDS_BRAVE_VPN_SHOW_FEEDBACK_MENU_ITEM);
  AddItemWithStringId(IDC_ABOUT_BRAVE_VPN, IDS_BRAVE_VPN_ABOUT_VPN_MENU_ITEM);
  AddItemWithStringId(IDC_MANAGE_BRAVE_VPN_PLAN,
                      IDS_BRAVE_VPN_MANAGE_MY_PLAN_MENU_ITEM);
}

bool BraveVPNMenuModel::IsItemCheckedAt(int index) const {
  ui::MenuModel::ItemType item_type = GetTypeAt(index);
  if (item_type == ui::MenuModel::TYPE_TOGGLE) {
    return IsCommandIdChecked(GetCommandIdAt(index));
  }

  return SimpleMenuModel::IsItemCheckedAt(index);
}

void BraveVPNMenuModel::ExecuteCommand(int command_id, int event_flags) {
  chrome::ExecuteCommand(browser_, command_id);
}

bool BraveVPNMenuModel::IsCommandIdChecked(int command_id) const {
  if (command_id != IDC_TOGGLE_BRAVE_VPN)
    return false;

  return IsVPNConnected();
}

bool BraveVPNMenuModel::IsVPNConnected() const {
  NOTIMPLEMENTED();
  return true;
}
