/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_TRAY_RUNNER_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_TRAY_RUNNER_H_

#include <memory>
#include <string>

#include "base/no_destructor.h"
#include "base/win/registry.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_icon/tray_menu_model.h"
#include "brave/components/brave_vpn/common/win/brave_windows_service_watcher.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ui {
class SimpleMenuModel;
}  // namespace ui

namespace brave_vpn {

class StatusTray;

class StatusTrayRunner : public TrayMenuModel::Delegate {
 public:
  static StatusTrayRunner* GetInstance();

  StatusTrayRunner(const StatusTrayRunner&) = delete;
  StatusTrayRunner& operator=(const StatusTrayRunner&) = delete;

  HRESULT Run();

  void SetTunnelServiceRunningForTesting(bool value) {
    service_running_for_testing_ = value;
  }

 private:
  friend class base::NoDestructor<StatusTrayRunner>;

  FRIEND_TEST_ALL_PREFIXES(StatusTrayRunnerTest, FindPakPath);

  StatusTrayRunner();
  ~StatusTrayRunner() override;

  bool IsTunnelServiceRunning() const;
  void SetupStatusIcon();
  void SignalExit();
  void UpdateIconState(bool connected, bool error);
  void SubscribeForServiceStopNotifications(const std::wstring& name);
  void SubscribeForStorageUpdates();
  void OnConnected(bool success);
  void OnDisconnected(bool success);
  void OnServiceStateChanged(int mask);
  void OnStorageUpdated();

  // TrayMenuModel::Delegate
  void ExecuteCommand(int command_id, int event_flags) override;
  void OnMenuWillShow(ui::SimpleMenuModel* source) override;

  std::unique_ptr<brave::ServiceWatcher> service_watcher_;
  absl::optional<bool> service_running_for_testing_;
  base::win::RegKey storage_;
  std::unique_ptr<StatusTray> status_tray_;
  base::OnceClosure quit_;
  base::WeakPtrFactory<StatusTrayRunner> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_TRAY_RUNNER_H_
