/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/named_third_party_registry_factory.h"

#include "brave/components/brave_perf_predictor/browser/named_third_party_registry.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_perf_predictor {

// static
NamedThirdPartyRegistryFactory* NamedThirdPartyRegistryFactory::GetInstance() {
  return base::Singleton<NamedThirdPartyRegistryFactory>::get();
}

NamedThirdPartyRegistry* NamedThirdPartyRegistryFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<NamedThirdPartyRegistry*>(
      NamedThirdPartyRegistryFactory::GetInstance()
          ->GetServiceForBrowserContext(context, true /*create*/));
}

NamedThirdPartyRegistryFactory::NamedThirdPartyRegistryFactory()
    : BrowserContextKeyedServiceFactory(
          "NamedThirdPartyRegistry",
          BrowserContextDependencyManager::GetInstance()) {}

NamedThirdPartyRegistryFactory::~NamedThirdPartyRegistryFactory() {}

KeyedService* NamedThirdPartyRegistryFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* registry = new NamedThirdPartyRegistry();
  registry->InitializeDefault();
  return registry;
}

bool NamedThirdPartyRegistryFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

}  // namespace brave_perf_predictor
