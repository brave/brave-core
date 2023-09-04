/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/wireguard/wireguard_service_observer.h"

#include "base/logging.h"

namespace brave_vpn {
namespace wireguard {

WireguardServiceObserver::WireguardServiceObserver() = default;
WireguardServiceObserver::~WireguardServiceObserver() = default;

bool WireguardServiceObserver::IsWireguardObserverActive() const {
  return service_watcher_ && service_watcher_->IsWatching();
}

void WireguardServiceObserver::SubscribeForWireguardNotifications(
    const std::wstring& name) {
  if (service_watcher_) {
    if (service_watcher_->GetServiceName() == name) {
      service_watcher_->StartWatching();
      return;
    }
  }
  service_watcher_ = std::make_unique<brave::ServiceWatcher>();
  if (!service_watcher_->Subscribe(
          name, SERVICE_NOTIFY_STOPPED,
          base::BindRepeating(
              &WireguardServiceObserver::OnWireguardServiceStateChanged,
              weak_factory_.GetWeakPtr()))) {
    VLOG(1) << "Unable to set service watcher for:" << name;
  }
}

void WireguardServiceObserver::StopWireguardObserver() {
  service_watcher_.reset();
}

}  // namespace wireguard
}  // namespace brave_vpn
