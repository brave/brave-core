/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_FACTORY_WIN_H_
#define BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_FACTORY_WIN_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

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
  friend base::NoDestructor<BraveVpnDnsObserverFactory>;

  BraveVpnDnsObserverFactory();
  ~BraveVpnDnsObserverFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_DNS_OBSERVER_FACTORY_WIN_H_
