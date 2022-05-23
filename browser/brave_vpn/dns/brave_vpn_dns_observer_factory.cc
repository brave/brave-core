/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_factory.h"

#include <memory>
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_vpn {

// static
BraveVpnDnsObserverFactory* BraveVpnDnsObserverFactory::GetInstance() {
  return base::Singleton<BraveVpnDnsObserverFactory>::get();
}

BraveVpnDnsObserverFactory::~BraveVpnDnsObserverFactory() = default;

BraveVpnDnsObserverFactory::BraveVpnDnsObserverFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnDNSObserverService",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* BraveVpnDnsObserverFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveVpnDnsObserverService(g_browser_process->local_state());
}

// static
BraveVpnDnsObserverService* BraveVpnDnsObserverFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsBraveVPNEnabled())
    return nullptr;
  return static_cast<BraveVpnDnsObserverService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

}  // namespace brave_vpn
