/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_service_factory_ios.h"

#include "brave/components/serp_metrics/serp_metrics_service.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_traits.h"

namespace serp_metrics {

namespace {
constexpr char kSerpMetricsTimePeriodStorage[] = "brave.stats.serp_metrics";
}  // namespace

SerpMetricsServiceFactoryIOS::SerpMetricsServiceFactoryIOS(PassKey key)
    : TypedProfileKeyedServiceFactoryIOS(
          std::move(key),
          "SerpMetricsService",
          ProfileSelection::kNoInstanceInIncognito,
          ServiceCreation::kCreateLazily,
          TestingCreation::kNoServiceForTests) {}

void SerpMetricsServiceFactoryIOS::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(kSerpMetricsTimePeriodStorage);
}

std::unique_ptr<KeyedService>
SerpMetricsServiceFactoryIOS::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return std::make_unique<SerpMetricsService>(
      *GetApplicationContext()->GetLocalState(),
      SerpMetricsPrefTimePeriodStoreFactory(profile->GetPrefs(),
                                            kSerpMetricsTimePeriodStorage));
}

}  // namespace serp_metrics
