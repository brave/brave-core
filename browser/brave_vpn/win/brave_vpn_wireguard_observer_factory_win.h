/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_OBSERVER_FACTORY_WIN_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_OBSERVER_FACTORY_WIN_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_vpn {

class BraveVpnWireguardObserverService;

class BraveVpnWireguardObserverFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  BraveVpnWireguardObserverFactory(const BraveVpnWireguardObserverFactory&) =
      delete;
  BraveVpnWireguardObserverFactory& operator=(
      const BraveVpnWireguardObserverFactory&) = delete;

  static BraveVpnWireguardObserverFactory* GetInstance();
  static BraveVpnWireguardObserverService* GetServiceForContext(
      content::BrowserContext* context);

 private:
  friend base::NoDestructor<BraveVpnWireguardObserverFactory>;

  BraveVpnWireguardObserverFactory();
  ~BraveVpnWireguardObserverFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_OBSERVER_FACTORY_WIN_H_
