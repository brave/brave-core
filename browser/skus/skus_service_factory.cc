// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/skus/skus_service_factory.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace skus {

// static
SkusServiceFactory* SkusServiceFactory::GetInstance() {
  static base::NoDestructor<SkusServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::SkusService> SkusServiceFactory::GetForContext(
    content::BrowserContext* context) {
  auto* instance = GetInstance()->GetServiceForBrowserContext(context, true);
  if (!instance) {
    return mojo::PendingRemote<mojom::SkusService>();
  }
  return static_cast<skus::SkusServiceImpl*>(instance)->MakeRemote();
}

// static
void SkusServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<skus::mojom::SkusService> receiver) {
  auto* service = static_cast<skus::SkusServiceImpl*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
  if (service) {
    service->Bind(std::move(receiver));
  }
}

SkusServiceFactory::SkusServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SkusService",
          BrowserContextDependencyManager::GetInstance()) {}

SkusServiceFactory::~SkusServiceFactory() = default;

std::unique_ptr<KeyedService>
SkusServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  // Return null if feature is disabled
  if (!base::FeatureList::IsEnabled(skus::features::kSkusFeature)) {
    return nullptr;
  }

  // Skus functionality not supported in private / Tor / guest windows
  if (!Profile::FromBrowserContext(context)->IsRegularProfile()) {
    return nullptr;
  }
  skus::MigrateSkusSettings(user_prefs::UserPrefs::Get(context),
                            g_browser_process->local_state());
  return std::make_unique<skus::SkusServiceImpl>(
      g_browser_process->local_state(),
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess());
}

void SkusServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  skus::RegisterProfilePrefsForMigration(registry);
}

}  // namespace skus
