/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/install_utils.h"

#include <windows.h>  // needed for WM_MENUCOMMAND

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/brave_vpn_tray_command_ids.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/icon_utils.h"

namespace brave_vpn {

bool UninstallStatusTrayIcon() {
  auto* hWnd = GetBraveVpnStatusTrayIconHWND();
  if (!hWnd) {
    return true;
  }

  return SendMessage(hWnd,
                     RegisterWindowMessage(kBraveVpnStatusTrayMessageName),
                     IDC_BRAVE_VPN_TRAY_EXIT, 0) == TRUE;
}

}  // namespace brave_vpn
