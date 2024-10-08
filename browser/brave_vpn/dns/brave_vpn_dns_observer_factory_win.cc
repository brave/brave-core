/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_factory_win.h"

#include <cstddef>
#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service_win.h"
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace brave_vpn {

// static
BraveVpnDnsObserverFactory* BraveVpnDnsObserverFactory::GetInstance() {
  static base::NoDestructor<BraveVpnDnsObserverFactory> instance;
  return instance.get();
}

BraveVpnDnsObserverFactory::~BraveVpnDnsObserverFactory() = default;

BraveVpnDnsObserverFactory::BraveVpnDnsObserverFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnDNSObserverService",
          BrowserContextDependencyManager::GetInstance()) {}

std::unique_ptr<KeyedService>
BraveVpnDnsObserverFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveVpnDnsObserverService>(
      g_browser_process->local_state(), user_prefs::UserPrefs::Get(context));
}

// static
BraveVpnDnsObserverService* BraveVpnDnsObserverFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!base::FeatureList::IsEnabled(
          brave_vpn::features::kBraveVPNDnsProtection)) {
    return nullptr;
  }
  DCHECK(brave_vpn::IsAllowedForContext(context));
  return static_cast<BraveVpnDnsObserverService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

void BraveVpnDnsObserverFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kBraveVpnShowDNSPolicyWarningDialog,
                                true);
}

}  // namespace brave_vpn
