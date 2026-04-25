/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_WIN_H_
#define BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_WIN_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#include "brave/components/brave_vpn/common/win/brave_windows_service_watcher.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace brave_vpn {

class BraveVpnDnsObserverService : public brave_vpn::BraveVPNServiceObserver,
                                   public KeyedService {
 public:
  explicit BraveVpnDnsObserverService(PrefService* local_state,
                                      PrefService* profile_prefs);
  ~BraveVpnDnsObserverService() override;
  BraveVpnDnsObserverService(const BraveVpnDnsObserverService&) = delete;
  BraveVpnDnsObserverService operator=(const BraveVpnDnsObserverService&) =
      delete;

  // brave_vpn::BraveVPNServiceObserver
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;

  void SetPolicyNotificationCallbackForTesting(base::OnceClosure callback) {
    policy_callback_ = std::move(callback);
  }

  void SetVPNNotificationCallbackForTesting(base::RepeatingClosure callback) {
    dialog_callback_ = std::move(callback);
  }
  void SetDNSHelperLiveForTesting(bool value) {
    dns_helper_live_for_testing_ = value;
  }
  bool IsVPNConnected() const;

 private:
  friend class BraveVpnDnsObserverServiceUnitTest;

  void OnServiceStopped(int mask);
  void RunServiceWatcher();
  void OnPrefChanged();
  bool IsDNSHelperLive();
  void LockDNS();
  void UnlockDNS();
  void ShowPolicyWarningMessage();
  void ShowVpnDnsSettingsNotificationDialog();
  void OnDnsModePrefChanged();

  void OnCheckIfServiceStarted();

  std::optional<brave_vpn::mojom::ConnectionState> connection_state_;
  std::unique_ptr<brave::ServiceWatcher> service_watcher_;
  std::optional<bool> dns_helper_live_for_testing_;
  base::OnceClosure policy_callback_;
  base::RepeatingClosure dialog_callback_;
  bool skip_notification_dialog_for_testing_ = false;
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;
  base::WeakPtrFactory<BraveVpnDnsObserverService> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_WIN_H_
