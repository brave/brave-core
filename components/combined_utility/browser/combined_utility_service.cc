/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/combined_utility/browser/combined_utility_service.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/service_process_host.h"

template <>
inline sandbox::mojom::Sandbox
content::GetServiceSandboxType<combined_utility::mojom::BatAdsLedgerFactory>() {
#if !BUILDFLAG(IS_ANDROID)
  return sandbox::mojom::Sandbox::kNoSandbox;
#else
  return sandbox::mojom::Sandbox::kUtility;
#endif  // !BUILDFLAG(IS_ANDROID)
}

namespace combined_utility {

namespace {

scoped_refptr<ServiceInstance> MakeBatAdsLedgerFactoryInstance() {
  mojo::Remote<combined_utility::mojom::BatAdsLedgerFactory>
      bat_ads_ledger_service;
  content::ServiceProcessHost::Launch(
      bat_ads_ledger_service.BindNewPipeAndPassReceiver(),
      content::ServiceProcessHost::Options()
          .WithDisplayName("TODO_NAME")
          .Pass());
  return base::MakeRefCounted<ServiceInstance>(
      std::move(bat_ads_ledger_service));
}

}  // namespace

ServiceInstance::ServiceInstance(
    mojo::Remote<combined_utility::mojom::BatAdsLedgerFactory> interface_remote)
    : interface_remote_(std::move(interface_remote)) {}

ServiceInstance::~ServiceInstance() = default;

scoped_refptr<ServiceInstance>
CombinedUtilityService::MakeStrongBatAdsLedgerFactory() {
  auto strong_factory_ref =
      base::WrapRefCounted(bat_ads_ledger_factory_weak_.get());
  if (!strong_factory_ref ||
      !strong_factory_ref->interface_remote().is_bound()) {
    strong_factory_ref = MakeBatAdsLedgerFactoryInstance();
    bat_ads_ledger_factory_weak_ = strong_factory_ref->AsWeakPtr();
  }
  return strong_factory_ref;
}

InterfaceHolder<bat_ledger::mojom::BatLedgerService>
CombinedUtilityService::MakeBatLedgerService() {
  auto strong_factory_ref = MakeStrongBatAdsLedgerFactory();
  mojo::Remote<bat_ledger::mojom::BatLedgerService> bat_ledger_service;
  strong_factory_ref->interface_remote()->MakeBatLedgerService(
      bat_ledger_service.BindNewPipeAndPassReceiver());
  return InterfaceHolder(std::move(bat_ledger_service),
                         std::move(strong_factory_ref));
}

InterfaceHolder<bat_ads::mojom::BatAdsService>
CombinedUtilityService::MakeBatAdsService() {
  auto strong_factory_ref = MakeStrongBatAdsLedgerFactory();
  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service;
  strong_factory_ref->interface_remote()->MakeBatAdsService(
      bat_ads_service.BindNewPipeAndPassReceiver());
  return InterfaceHolder(std::move(bat_ads_service),
                         std::move(strong_factory_ref));
}

CombinedUtilityService::CombinedUtilityService(
    content::BrowserContext* /*browser_context*/) {}

CombinedUtilityService::~CombinedUtilityService() = default;

// static
CombinedUtilityServiceFactory* CombinedUtilityServiceFactory::GetInstance() {
  return base::Singleton<CombinedUtilityServiceFactory>::get();
}

CombinedUtilityService* CombinedUtilityServiceFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<CombinedUtilityService*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

CombinedUtilityServiceFactory::CombinedUtilityServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "CombinedUtilityServiceFactory",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* CombinedUtilityServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new CombinedUtilityService(context);
}

}  // namespace combined_utility
