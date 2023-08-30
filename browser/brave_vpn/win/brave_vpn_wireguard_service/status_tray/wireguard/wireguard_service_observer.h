/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_WIREGUARD_WIREGUARD_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_WIREGUARD_WIREGUARD_SERVICE_OBSERVER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/common/win/brave_windows_service_watcher.h"

namespace brave_vpn {
namespace wireguard {

class WireguardServiceObserver {
 public:
  WireguardServiceObserver();

  WireguardServiceObserver(const WireguardServiceObserver&) = delete;
  WireguardServiceObserver& operator=(const WireguardServiceObserver&) = delete;

  virtual ~WireguardServiceObserver();

  virtual void OnWireguardServiceStateChanged(int mask) = 0;

  void SubscribeForWireguardNotifications(const std::wstring& name);
  bool IsWireguardObserverActive() const;
  void StopWireguardObserver();

 private:
  std::unique_ptr<brave::ServiceWatcher> service_watcher_;
  base::WeakPtrFactory<WireguardServiceObserver> weak_factory_{this};
};

}  // namespace wireguard
}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_WIREGUARD_WIREGUARD_SERVICE_OBSERVER_H_
