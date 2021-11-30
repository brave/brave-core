/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_federated_learning/brave_federated_learning_service_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_federated_learning/brave_federated_learning_service.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

// static
BraveFederatedLearningService*
BraveFederatedLearningServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BraveFederatedLearningService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BraveFederatedLearningServiceFactory*
BraveFederatedLearningServiceFactory::GetInstance() {
  return base::Singleton<BraveFederatedLearningServiceFactory>::get();
}

BraveFederatedLearningServiceFactory::BraveFederatedLearningServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveFederatedLearningService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveFederatedLearningServiceFactory::~BraveFederatedLearningServiceFactory() {}

KeyedService* BraveFederatedLearningServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto url_loader_factory = context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess();
  std::unique_ptr<BraveFederatedLearningService>
      brave_federated_learning_service(new BraveFederatedLearningService(
          user_prefs::UserPrefs::Get(context), g_browser_process->local_state(),
          url_loader_factory));
  return brave_federated_learning_service.release();
}

void BraveFederatedLearningServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  BraveFederatedLearningService::RegisterProfilePrefs(registry);
}

bool BraveFederatedLearningServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool BraveFederatedLearningServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace brave
