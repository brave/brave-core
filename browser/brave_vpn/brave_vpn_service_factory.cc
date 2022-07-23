/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"

#include "base/feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "brave/components/skus/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_factory.h"
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"
#endif

namespace brave_vpn {

// static
BraveVpnServiceFactory* BraveVpnServiceFactory::GetInstance() {
  return base::Singleton<BraveVpnServiceFactory>::get();
}

// static
BraveVpnService* BraveVpnServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BraveVpnService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
#if !BUILDFLAG(IS_ANDROID)
void BraveVpnServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  auto* service = static_cast<BraveVpnService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
  if (service) {
    service->BindInterface(std::move(receiver));
  }
}
#endif

BraveVpnServiceFactory::BraveVpnServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(skus::SkusServiceFactory::GetInstance());
#if BUILDFLAG(IS_WIN)
  DependsOn(brave_vpn::BraveVpnDnsObserverFactory::GetInstance());
#endif
}

BraveVpnServiceFactory::~BraveVpnServiceFactory() = default;

KeyedService* BraveVpnServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // TODO(simonhong): Can we use this check for android?
  // For now, vpn is disabled by default on desktop but not sure on
  // android.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  if (!IsBraveVPNEnabled())
    return nullptr;
#endif

  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();

  auto callback = base::BindRepeating(
      [](content::BrowserContext* context) {
        return skus::SkusServiceFactory::GetForContext(context);
      },
      context);
  auto* vpn_service = new BraveVpnService(
      shared_url_loader_factory, user_prefs::UserPrefs::Get(context), callback);
#if BUILDFLAG(IS_WIN)
  auto* dns_observer_service =
      brave_vpn::BraveVpnDnsObserverFactory::GetInstance()
          ->GetServiceForContext(context);
  if (dns_observer_service)
    dns_observer_service->Observe(vpn_service);
#endif
  return vpn_service;
}

}  // namespace brave_vpn
