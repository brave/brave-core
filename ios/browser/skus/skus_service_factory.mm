/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/skus/skus_service_factory.h"

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace skus {

// static
mojo::PendingRemote<mojom::SkusService> SkusServiceFactory::GetForProfile(
    ProfileIOS* profile) {
  auto* service = GetInstance()->GetServiceForProfileAs<skus::SkusServiceImpl>(
      profile, true);
  if (!service) {
    return mojo::PendingRemote<mojom::SkusService>();
  }
  return service->MakeRemote();
}

// static
SkusServiceFactory* SkusServiceFactory::GetInstance() {
  static base::NoDestructor<SkusServiceFactory> instance;
  return instance.get();
}

SkusServiceFactory::SkusServiceFactory()
    : ProfileKeyedServiceFactoryIOS("SkusService",
                                    TestingCreation::kNoServiceForTests) {}

SkusServiceFactory::~SkusServiceFactory() = default;

std::unique_ptr<KeyedService> SkusServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  // Return null if feature is disabled
  if (!base::FeatureList::IsEnabled(skus::features::kSkusFeature)) {
    return nullptr;
  }

  auto* profile = ProfileIOS::FromBrowserState(context);
  if (profile->IsOffTheRecord()) {
    return nullptr;
  }
  skus::MigrateSkusSettings(profile->GetPrefs(),
                            GetApplicationContext()->GetLocalState());
  std::unique_ptr<skus::SkusServiceImpl> sku_service(
      new skus::SkusServiceImpl(GetApplicationContext()->GetLocalState(),
                                profile->GetSharedURLLoaderFactory()));
  return sku_service;
}

void SkusServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  skus::RegisterProfilePrefsForMigration(registry);
}

}  // namespace skus
