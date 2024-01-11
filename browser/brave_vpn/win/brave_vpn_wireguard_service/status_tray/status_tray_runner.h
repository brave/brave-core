/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_TRAY_RUNNER_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_TRAY_RUNNER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/win/registry.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/tray_menu_model.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/wireguard/wireguard_service_observer.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/win/ras/ras_connection_observer.h"
#include "components/version_info/channel.h"

namespace ui {
class SimpleMenuModel;
}  // namespace ui

namespace brave_vpn {

class StatusTray;

class StatusTrayRunner : public TrayMenuModel::Delegate,
                         public wireguard::WireguardServiceObserver,
                         public ras::RasConnectionObserver {
 public:
  using SetIconStateCallback =
      base::RepeatingCallback<void(int icon_id, int tooltip_id)>;

  static StatusTrayRunner* GetInstance();

  StatusTrayRunner(const StatusTrayRunner&) = delete;
  StatusTrayRunner& operator=(const StatusTrayRunner&) = delete;

  HRESULT Run();

  void SetVPNConnectedForTesting(bool value) {
    vpn_connected_for_testing_ = value;
  }

  void SetIconStateCallbackForTesting(SetIconStateCallback callback) {
    callback_for_testing_ = std::move(callback);
  }

  void SetCurrentStateForTesting(
      std::optional<brave_vpn::mojom::ConnectionState> state) {
    current_state_ = std::move(state);
  }

 private:
  friend class base::NoDestructor<StatusTrayRunner>;

  FRIEND_TEST_ALL_PREFIXES(StatusTrayRunnerTest, FindPakPath);
  FRIEND_TEST_ALL_PREFIXES(StatusTrayRunnerTest, UpdateConnectionState);
  friend class StatusTrayRunnerTest;

  StatusTrayRunner();
  ~StatusTrayRunner() override;

  // ras::RasConnectionObserver
  void OnRasConnectionStateChanged() override;

  // WireguardServiceObserver overrides:
  void OnWireguardServiceStateChanged(int mask) override;

  void SetupStatusIcon();
  void SignalExit();
  void UpdateConnectionState();
  void SubscribeForServiceStopNotifications(const std::wstring& name);
  void SubscribeForStorageUpdates();
  void OnConnected(bool success);
  void OnDisconnected(bool success);

  void SetupConnectionObserver();
  void OnStorageUpdated();
  bool IsIconCreated();
  void SetIconState(int icon_id, int tooltip_id);
  bool IsVPNConnected() const;
  brave_vpn::mojom::ConnectionState GetConnectionState();
  void ConnectVPN();
  void DisconnectVPN();

  // TrayMenuModel::Delegate
  void ExecuteCommand(int command_id, int event_flags) override;
  void OnMenuWillShow(ui::SimpleMenuModel* source) override;

  version_info::Channel channel_ = version_info::Channel::UNKNOWN;
  std::optional<brave_vpn::mojom::ConnectionState> current_state_;
  SetIconStateCallback callback_for_testing_;
  std::optional<bool> vpn_connected_for_testing_;
  base::win::RegKey storage_;
  std::unique_ptr<StatusTray> status_tray_;
  base::OnceClosure quit_;

  base::WeakPtrFactory<StatusTrayRunner> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_TRAY_RUNNER_H_
