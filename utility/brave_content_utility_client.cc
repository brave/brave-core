/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/brave_content_utility_client.h"

#include <memory>
#include <utility>

#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/services/bat_ads/bat_ads_service_impl.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/service_factory.h"

#if !defined(OS_ANDROID)
#include "brave/utility/importer/brave_profile_import_impl.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/services/ipfs/ipfs_service_impl.h"
#include "brave/components/services/ipfs/public/mojom/ipfs_service.mojom.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "brave/components/services/tor/tor_launcher_impl.h"
#endif

namespace {

#if !defined(OS_ANDROID)
auto RunBraveProfileImporter(
    mojo::PendingReceiver<brave::mojom::ProfileImport> receiver) {
  return std::make_unique<BraveProfileImportImpl>(std::move(receiver));
}
#endif

#if BUILDFLAG(ENABLE_IPFS)
auto RunIpfsService(mojo::PendingReceiver<ipfs::mojom::IpfsService> receiver) {
  return std::make_unique<ipfs::IpfsServiceImpl>(std::move(receiver));
}
#endif

#if BUILDFLAG(ENABLE_TOR)
auto RunTorLauncher(mojo::PendingReceiver<tor::mojom::TorLauncher> receiver) {
  return std::make_unique<tor::TorLauncherImpl>(std::move(receiver));
}
#endif

auto RunBatLedgerService(
    mojo::PendingReceiver<bat_ledger::mojom::BatLedgerService> receiver) {
  return std::make_unique<bat_ledger::BatLedgerServiceImpl>(
      std::move(receiver));
}

auto RunBatAdsService(
    mojo::PendingReceiver<bat_ads::mojom::BatAdsService> receiver) {
  return std::make_unique<bat_ads::BatAdsServiceImpl>(std::move(receiver));
}

}  // namespace

BraveContentUtilityClient::BraveContentUtilityClient() = default;
BraveContentUtilityClient::~BraveContentUtilityClient() = default;

void BraveContentUtilityClient::RegisterMainThreadServices(
    mojo::ServiceFactory& services) {
#if !defined(OS_ANDROID)
  services.Add(RunBraveProfileImporter);
#endif

#if BUILDFLAG(ENABLE_IPFS)
  services.Add(RunIpfsService);
#endif

#if BUILDFLAG(ENABLE_TOR)
  services.Add(RunTorLauncher);
#endif

  services.Add(RunBatLedgerService);

  services.Add(RunBatAdsService);

  return ChromeContentUtilityClient::RegisterMainThreadServices(services);
}
