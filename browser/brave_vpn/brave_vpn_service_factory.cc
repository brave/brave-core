/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"

#include <utility>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/misc_metrics/uptime_monitor_impl.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/skus/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN_V1)
#include "brave/components/brave_vpn/browser/brave_vpn_service_impl.h"
#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_factory_win.h"
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service_win.h"
#include "brave/browser/brave_vpn/win/brave_vpn_service_delegate_win.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_observer_factory_win.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_observer_service_win.h"
#endif  // IS_WIN
#endif  // ENABLE_BRAVE_VPN_V1

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2)
#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2)

namespace brave_vpn {
namespace {

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2)

std::unique_ptr<KeyedService> BuildVpnService_V2(
    content::BrowserContext* context) {
  auto* local_state = g_browser_process->local_state();
  auto* profile_prefs = user_prefs::UserPrefs::Get(context);
  brave_vpn::MigrateVPNSettings(profile_prefs, local_state);

  // Return stub implementation.
  return std::make_unique<v2::BraveVpnServiceImpl>(local_state, profile_prefs);
}

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2)

#if BUILDFLAG(ENABLE_BRAVE_VPN_V1)

std::unique_ptr<KeyedService> BuildVpnService_V1(
    content::BrowserContext* context) {
#if !BUILDFLAG(IS_ANDROID)
  if (!g_brave_browser_process->brave_vpn_connection_manager()) {
    return nullptr;
  }
#endif

  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  auto* local_state = g_browser_process->local_state();
  brave_vpn::MigrateVPNSettings(user_prefs::UserPrefs::Get(context),
                                local_state);
  auto callback = base::BindRepeating(
      [](content::BrowserContext* context) {
        return skus::SkusServiceFactory::GetForContext(context);
      },
      context);

  // Get the UptimeMonitor from ProcessMiscMetrics
  base::WeakPtr<misc_metrics::UptimeMonitor> uptime_monitor = nullptr;
  if (g_brave_browser_process &&
      g_brave_browser_process->process_misc_metrics() &&
      g_brave_browser_process->process_misc_metrics()->uptime_monitor()) {
    uptime_monitor = g_brave_browser_process->process_misc_metrics()
                         ->uptime_monitor()
                         ->GetWeakPtr();
  }

  auto vpn_service = std::make_unique<BraveVpnServiceImpl>(
      g_brave_browser_process->brave_vpn_connection_manager(),
      shared_url_loader_factory, local_state,
      user_prefs::UserPrefs::Get(context), uptime_monitor, callback);
#if BUILDFLAG(IS_WIN)
  vpn_service->set_delegate(std::make_unique<BraveVPNServiceDelegateWin>());
  if (auto* wg_observer_service =
          brave_vpn::BraveVpnWireguardObserverFactory::GetInstance()
              ->GetServiceForContext(context)) {
    wg_observer_service->Observe(vpn_service.get());
  }
  if (auto* dns_observer_service =
          brave_vpn::BraveVpnDnsObserverFactory::GetInstance()
              ->GetServiceForContext(context)) {
    dns_observer_service->Observe(vpn_service.get());
  }
#endif

  return vpn_service;
}

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V1)

std::unique_ptr<KeyedService> BuildVpnService(
    content::BrowserContext* context) {
  if (!brave_vpn::IsAllowedForContext(context)) {
    return nullptr;
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2)
  if (base::FeatureList::IsEnabled(features::kBraveVPNExperimentalV2)) {
    return BuildVpnService_V2(context);
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN_V1)
  return BuildVpnService_V1(context);
#else
  NOTREACHED() << "No VPN implementation available";
#endif
}

}  // namespace

// static
BraveVpnServiceFactory* BraveVpnServiceFactory::GetInstance() {
  static base::NoDestructor<BraveVpnServiceFactory> instance;
  return instance.get();
}

#if BUILDFLAG(IS_ANDROID)
// static
mojo::PendingRemote<brave_vpn::mojom::ServiceHandler>
BraveVpnServiceFactory::GetRemoteForProfile(Profile* profile) {
  auto* service = static_cast<BraveVpnService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));

  if (!service) {
    return mojo::PendingRemote<brave_vpn::mojom::ServiceHandler>();
  }

  return service->MakeRemote();
}
#endif  // BUILDFLAG(IS_ANDROID)

// static
BraveVpnService* BraveVpnServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BraveVpnService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
void BraveVpnServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  auto* service = static_cast<BraveVpnService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
  if (service) {
    service->BindInterface(std::move(receiver));
  }
}

BraveVpnServiceFactory::BraveVpnServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(skus::SkusServiceFactory::GetInstance());
#if BUILDFLAG(ENABLE_BRAVE_VPN_V1) && BUILDFLAG(IS_WIN)
  DependsOn(brave_vpn::BraveVpnWireguardObserverFactory::GetInstance());
  DependsOn(brave_vpn::BraveVpnDnsObserverFactory::GetInstance());
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V1) && BUILDFLAG(IS_WIN)
}

BraveVpnServiceFactory::~BraveVpnServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveVpnServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return BuildVpnService(context);
}

// static
BrowserContextKeyedServiceFactory::TestingFactory
BraveVpnServiceFactory::GetDefaultFactory() {
  return base::BindRepeating(&BuildVpnService);
}

void BraveVpnServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  brave_vpn::RegisterProfilePrefs(registry);
}

}  // namespace brave_vpn
