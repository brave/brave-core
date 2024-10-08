/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_observer_factory_win.h"

#include <memory>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_observer_service_win.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace brave_vpn {

// static
BraveVpnWireguardObserverFactory*
BraveVpnWireguardObserverFactory::GetInstance() {
  static base::NoDestructor<BraveVpnWireguardObserverFactory> instance;
  return instance.get();
}

BraveVpnWireguardObserverFactory::~BraveVpnWireguardObserverFactory() = default;

BraveVpnWireguardObserverFactory::BraveVpnWireguardObserverFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnWireguardObserverService",
          BrowserContextDependencyManager::GetInstance()) {}

std::unique_ptr<KeyedService>
BraveVpnWireguardObserverFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveVpnWireguardObserverService>();
}

// static
BraveVpnWireguardObserverService*
BraveVpnWireguardObserverFactory::GetServiceForContext(
    content::BrowserContext* context) {
  DCHECK(brave_vpn::IsAllowedForContext(context));
  return static_cast<BraveVpnWireguardObserverService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

}  // namespace brave_vpn
