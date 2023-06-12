/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/brave_vpn_menu_model.h"

#include "brave/components/brave_vpn/browser/connection/common/win/utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/brave_vpn_interactive_strings_en.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/brave_vpn_tray_command_ids.h"
#include "ui/gfx/image/image.h"

namespace {
std::u16string GetVpnStatusLabel(bool active) {
  std::u16string label = brave::kBraveVpnStatusItemName;
  label += (active ? brave::kBraveVpnActiveText : brave::kBraveVpnInactiveText);
  return label;
}
}  // namespace

struct BraveVpnMenuModel::ItemState {
  ItemState()
      : checked(false), enabled(true), visible(true), is_dynamic(false) {}
  bool checked;
  bool enabled;
  bool visible;
  bool is_dynamic;
  ui::Accelerator accelerator;
  std::u16string label;
  gfx::Image icon;
};

////////////////////////////////////////////////////////////////////////////////
// BraveVpnMenuModel, public:

BraveVpnMenuModel::BraveVpnMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(this), delegate_(delegate) {}

BraveVpnMenuModel::~BraveVpnMenuModel() {}

std::u16string BraveVpnMenuModel::GetLabelForCommandId(int command_id) const {
  auto iter = item_states_.find(command_id);
  if (iter != item_states_.end()) {
    return iter->second.label;
  }
  return std::u16string();
}

ui::ImageModel BraveVpnMenuModel::GetIconForCommandId(int command_id) const {
  auto iter = item_states_.find(command_id);
  if (iter != item_states_.end() && !iter->second.icon.IsEmpty()) {
    return ui::ImageModel::FromImage(iter->second.icon);
  }
  return ui::ImageModel();
}

void BraveVpnMenuModel::ExecuteCommand(int command_id, int event_flags) {
  if (delegate_) {
    delegate_->ExecuteCommand(command_id, event_flags);
  }
}

void BraveVpnMenuModel::RebuildMenu() {
  Clear();
  item_states_.clear();
  auto active = brave_vpn::IsWindowsServiceRunning(
      brave_vpn::GetBraveVpnWireguardTunnelServiceName());
  AddItem(IDC_BRAVE_VPN_TRAY_STATUS_ITEM, GetVpnStatusLabel(active));
  SetEnabledAt(0, false);
  if (active) {
    AddItem(IDC_BRAVE_VPN_TRAY_DISCONNECT_VPN_ITEM,
            brave::kBraveVpnDisconnectItemName);
  } else {
    AddItem(IDC_BRAVE_VPN_TRAY_CONNECT_VPN_ITEM,
            brave::kBraveVpnConnectItemName);
  }
  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItem(IDC_BRAVE_VPN_TRAY_MANAGE_ACCOUNT_ITEM,
          brave::kBraveVpnManageAccountItemName);
  AddItem(IDC_BRAVE_VPN_TRAY_ABOUT_ITEM, brave::kBraveVpnAboutItemName);
  //AddSeparator(ui::NORMAL_SEPARATOR);
  // AddItem(IDC_BRAVE_VPN_TRAY_EXIT_ICON, brave::kBraveVpnRemoveItemName);
}
