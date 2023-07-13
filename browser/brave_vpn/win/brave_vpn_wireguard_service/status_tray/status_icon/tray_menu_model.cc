/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/tray_menu_model.h"

////////////////////////////////////////////////////////////////////////////////
// TrayMenuModel, public:
namespace brave_vpn {

TrayMenuModel::TrayMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(this), delegate_(delegate) {}

TrayMenuModel::~TrayMenuModel() {}

void TrayMenuModel::ExecuteCommand(int command_id, int event_flags) {
  if (delegate_) {
    delegate_->ExecuteCommand(command_id, event_flags);
  }
}

void TrayMenuModel::OnMenuWillShow(ui::SimpleMenuModel* source) {
  if (delegate_) {
    delegate_->OnMenuWillShow(source);
  }
}

}  // namespace brave_vpn
