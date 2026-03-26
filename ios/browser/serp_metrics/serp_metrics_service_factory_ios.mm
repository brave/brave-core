/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_service_factory_ios.h"

#include "base/no_destructor.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_prefs.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_service_ios.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/core/keyed_service_base_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace serp_metrics {

// static
SerpMetricsServiceFactoryIOS* SerpMetricsServiceFactoryIOS::GetInstance() {
  static base::NoDestructor<SerpMetricsServiceFactoryIOS> instance;
  return instance.get();
}

// static
SerpMetrics* SerpMetricsServiceFactoryIOS::GetForProfile(ProfileIOS* profile) {
  SerpMetricsServiceIOS* service =
      GetInstance()->GetServiceForProfileAs<SerpMetricsServiceIOS>(
          profile, /*create=*/true);
  return service ? service->Get() : nullptr;
}

SerpMetricsServiceFactoryIOS::SerpMetricsServiceFactoryIOS()
    : ProfileKeyedServiceFactoryIOS("SerpMetricsService",
                                    ProfileSelection::kNoInstanceInIncognito) {}

SerpMetricsServiceFactoryIOS::~SerpMetricsServiceFactoryIOS() = default;

void SerpMetricsServiceFactoryIOS::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(kSerpMetricsTimePeriodStorage);
}

std::unique_ptr<KeyedService>
SerpMetricsServiceFactoryIOS::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return std::make_unique<SerpMetricsServiceIOS>(
      *GetApplicationContext()->GetLocalState(), *profile->GetPrefs());
}

}  // namespace serp_metrics
