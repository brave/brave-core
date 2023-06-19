/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/skus/skus_service_factory.h"

#include "base/feature_list.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace skus {

// static
mojo::PendingRemote<mojom::SkusService> SkusServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  auto* service = GetInstance()->GetServiceForBrowserState(browser_state, true);
  if (!service) {
    return mojo::PendingRemote<mojom::SkusService>();
  }
  return static_cast<skus::SkusServiceImpl*>(service)->MakeRemote();
}

// static
SkusServiceFactory* SkusServiceFactory::GetInstance() {
  return base::Singleton<SkusServiceFactory>::get();
}

SkusServiceFactory::SkusServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "SkusService",
          BrowserStateDependencyManager::GetInstance()) {}

SkusServiceFactory::~SkusServiceFactory() = default;

std::unique_ptr<KeyedService> SkusServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  // Return null if feature is disabled
  if (!base::FeatureList::IsEnabled(skus::features::kSkusFeature)) {
    return nullptr;
  }

  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  if (browser_state->IsOffTheRecord()) {
    return nullptr;
  }
  std::unique_ptr<skus::SkusServiceImpl> sku_service(new skus::SkusServiceImpl(
      browser_state->GetPrefs(), browser_state->GetSharedURLLoaderFactory()));
  return sku_service;
}

void SkusServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  skus::RegisterProfilePrefsForMigration(registry);
}

bool SkusServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace skus
