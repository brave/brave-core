/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/brave_content_utility_client.h"

#include "brave/common/tor/tor_launcher.mojom.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/utility/tor/tor_launcher_service.h"

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/services/bat_ads/bat_ads_app.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/services/bat_ledger/bat_ledger_app.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#endif

BraveContentUtilityClient::BraveContentUtilityClient()
    : ChromeContentUtilityClient() {}

BraveContentUtilityClient::~BraveContentUtilityClient() = default;

void BraveContentUtilityClient::RegisterServices(
    ChromeContentUtilityClient::StaticServiceMap* services) {
  ChromeContentUtilityClient::RegisterServices(services);

  service_manager::EmbeddedServiceInfo tor_launcher_info;
  tor_launcher_info.factory = base::BindRepeating(
      &tor::TorLauncherService::CreateService);
  services->emplace(tor::mojom::kTorLauncherServiceName, tor_launcher_info);

#if BUILDFLAG(BRAVE_ADS_ENABLED)
  service_manager::EmbeddedServiceInfo bat_ads_service;
  bat_ads_service.factory = base::BindRepeating(
      &bat_ads::BatAdsApp::CreateService);
  services->emplace(bat_ads::mojom::kServiceName, bat_ads_service);
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  service_manager::EmbeddedServiceInfo bat_ledger_info;
  bat_ledger_info.factory = base::BindRepeating(
    &bat_ledger::BatLedgerApp::CreateService);
  services->emplace(bat_ledger::mojom::kServiceName, bat_ledger_info);
#endif
}
