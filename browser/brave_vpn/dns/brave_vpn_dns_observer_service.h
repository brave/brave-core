/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/brave_vpn_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_config_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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

 private:
  friend class BraveVpnDnsObserverServiceUnitTest;

  void OnPrefChanged();
  void LockDNS();
  void UnlockDNS();
  void ShowPolicyWarningMessage();
  void ShowVpnDnsSettingsNotificationDialog();
  void OnDnsModePrefChanged();

  base::OnceClosure policy_callback_;
  base::RepeatingClosure dialog_callback_;
  bool skip_notification_dialog_for_testing_ = false;
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;

  base::WeakPtrFactory<BraveVpnDnsObserverService> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_H_
