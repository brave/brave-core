/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_INTERACTIVE_MAIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_INTERACTIVE_MAIN_H_

#include <memory>

#include "base/no_destructor.h"
#include "base/win/windows_types.h"

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/brave_vpn_menu_model.h"

class StatusIconWin;
class StatusTrayWin;

namespace brave_vpn {

class InteractiveMain : public BraveVpnMenuModel::Delegate {
 public:
  static InteractiveMain* GetInstance();

  InteractiveMain(const InteractiveMain&) = delete;
  InteractiveMain& operator=(const InteractiveMain&) = delete;

  void SetupStatusIcon();

  HRESULT Run();
  void SignalExit();
  // BraveVpnMenuModel::Delegate
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  friend class base::NoDestructor<InteractiveMain>;

  InteractiveMain();
  ~InteractiveMain() override;

  void OnConnect(bool success);
  void OnDisconnect(bool success);

  std::unique_ptr<StatusTrayWin> status_tray_;
  // Reference to our status icon (if any) - owned by the StatusTray.
  raw_ptr<StatusIconWin, DanglingUntriaged> status_icon_ = nullptr;

  base::OnceClosure quit_;
  base::WeakPtrFactory<InteractiveMain> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_INTERACTIVE_MAIN_H_
