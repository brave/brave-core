/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/brave_vpn_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class SecureDnsConfig;
namespace content {
class BrowserContext;
}  // namespace content

namespace brave_vpn {

class BraveVpnDnsObserverService : public brave_vpn::BraveVPNServiceObserver,
                                   public KeyedService {
 public:
  explicit BraveVpnDnsObserverService(PrefService* local_state);
  ~BraveVpnDnsObserverService() override;
  BraveVpnDnsObserverService(const BraveVpnDnsObserverService&) = delete;
  BraveVpnDnsObserverService operator=(const BraveVpnDnsObserverService&) =
      delete;

  // brave_vpn::BraveVPNServiceObserver
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;
  bool ShouldAllowExternalChanges() const;
  void SetAllowExternalChangesForTesting(bool allow) {
    allow_changes_for_testing_ = allow;
  }

 private:
  void OnDNSPrefChanged();

  void SetDNSOverHTTPSMode(const std::string& mode,
                           const std::string& doh_providers);
  bool active_ = false;
  absl::optional<bool> allow_changes_for_testing_;
  raw_ptr<PrefService> local_state_;
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<SecureDnsConfig> user_dns_config_;
  base::WeakPtrFactory<BraveVpnDnsObserverService> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_SERVICE_H_
