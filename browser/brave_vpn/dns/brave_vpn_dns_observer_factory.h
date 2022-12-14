/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_vpn {

class BraveVpnDnsObserverService;

class BraveVpnDnsObserverFactory : public BrowserContextKeyedServiceFactory {
 public:
  BraveVpnDnsObserverFactory(const BraveVpnDnsObserverFactory&) = delete;
  BraveVpnDnsObserverFactory& operator=(const BraveVpnDnsObserverFactory&) =
      delete;

  static BraveVpnDnsObserverFactory* GetInstance();
  static BraveVpnDnsObserverService* GetServiceForContext(
      content::BrowserContext* context);
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

 private:
  friend struct base::DefaultSingletonTraits<BraveVpnDnsObserverFactory>;

  BraveVpnDnsObserverFactory();
  ~BraveVpnDnsObserverFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_FACTORY_H_
