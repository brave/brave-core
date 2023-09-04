/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_federated/brave_federated_service_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_federated/brave_federated_service.h"
#include "brave/components/brave_federated/features.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_federated {

// static
BraveFederatedService* BraveFederatedServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BraveFederatedService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BraveFederatedServiceFactory* BraveFederatedServiceFactory::GetInstance() {
  static base::NoDestructor<BraveFederatedServiceFactory> instance;
  return instance.get();
}

BraveFederatedServiceFactory::BraveFederatedServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveFederatedService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveFederatedServiceFactory::~BraveFederatedServiceFactory() = default;

KeyedService* BraveFederatedServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(features::kFederatedLearning)) {
    return nullptr;
  }

  auto url_loader_factory = context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess();
  std::unique_ptr<BraveFederatedService> brave_federated_service(
      new BraveFederatedService(user_prefs::UserPrefs::Get(context),
                                g_browser_process->local_state(),
                                context->GetPath(), url_loader_factory));
  return brave_federated_service.release();
}

void BraveFederatedServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  BraveFederatedService::RegisterProfilePrefs(registry);
}

bool BraveFederatedServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

bool BraveFederatedServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace brave_federated
