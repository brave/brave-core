/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/brave_content_utility_client.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/tor/buildflags.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/services/bat_ads/bat_ads_app.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/services/bat_ledger/bat_ledger_app.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "brave/components/services/tor/tor_launcher_service.h"
#endif

BraveContentUtilityClient::BraveContentUtilityClient()
    : ChromeContentUtilityClient() {}

BraveContentUtilityClient::~BraveContentUtilityClient() = default;

namespace {

#if BUILDFLAG(BRAVE_ADS_ENABLED) || BUILDFLAG(BRAVE_REWARDS_ENABLED) || \
    BUILDFLAG(ENABLE_TOR)
void RunServiceAsyncThenTerminateProcess(
    std::unique_ptr<service_manager::Service> service) {
  service_manager::Service::RunAsyncUntilTermination(
      std::move(service),
      base::BindOnce([] { content::UtilityThread::Get()->ReleaseProcess(); }));
}
#endif

#if BUILDFLAG(ENABLE_TOR)
std::unique_ptr<service_manager::Service> CreateTorLauncherService(
    mojo::PendingReceiver<service_manager::mojom::Service> receiver) {
  return std::make_unique<tor::TorLauncherService>(
      std::move(receiver));
}
#endif

#if BUILDFLAG(BRAVE_ADS_ENABLED)
std::unique_ptr<service_manager::Service> CreateBatAdsService(
    mojo::PendingReceiver<service_manager::mojom::Service> receiver) {
  return std::make_unique<bat_ads::BatAdsApp>(
      std::move(receiver));
}
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
std::unique_ptr<service_manager::Service> CreateBatLedgerService(
    mojo::PendingReceiver<service_manager::mojom::Service> receiver) {
  return std::make_unique<bat_ledger::BatLedgerApp>(
      std::move(receiver));
}
#endif

}  // namespace

bool BraveContentUtilityClient::HandleServiceRequest(
    const std::string& service_name,
    mojo::PendingReceiver<service_manager::mojom::Service> receiver) {

#if BUILDFLAG(ENABLE_TOR)
  if (service_name == tor::mojom::kServiceName) {
    RunServiceAsyncThenTerminateProcess(
        CreateTorLauncherService(std::move(receiver)));
    return true;
  }
#endif

#if BUILDFLAG(BRAVE_ADS_ENABLED)
  if (service_name == bat_ads::mojom::kServiceName) {
    RunServiceAsyncThenTerminateProcess(
        CreateBatAdsService(std::move(receiver)));
    return true;
  }
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  if (service_name == bat_ledger::mojom::kServiceName) {
    RunServiceAsyncThenTerminateProcess(
        CreateBatLedgerService(std::move(receiver)));
    return true;
  }
#endif

  return ChromeContentUtilityClient::HandleServiceRequest(
      service_name, std::move(receiver));
}
