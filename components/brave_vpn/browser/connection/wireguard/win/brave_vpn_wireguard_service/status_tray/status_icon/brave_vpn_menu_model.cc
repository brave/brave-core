/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/status_icon/brave_vpn_menu_model.h"

////////////////////////////////////////////////////////////////////////////////
// BraveVpnMenuModel, public:
namespace brave_vpn {

BraveVpnMenuModel::BraveVpnMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(this), delegate_(delegate) {}

BraveVpnMenuModel::~BraveVpnMenuModel() {}

void BraveVpnMenuModel::ExecuteCommand(int command_id, int event_flags) {
  if (delegate_) {
    delegate_->ExecuteCommand(command_id, event_flags);
  }
}

void BraveVpnMenuModel::OnMenuWillShow(ui::SimpleMenuModel* source) {
  if (delegate_) {
    delegate_->OnMenuWillShow(source);
  }
}

}  // namespace brave_vpn
