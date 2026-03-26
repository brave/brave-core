/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_service_factory.h"

#include <memory>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/no_destructor.h"
#include "brave/browser/serp_metrics/serp_metrics_service.h"
#include "brave/browser/serp_metrics/serp_metrics_time_period_store_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

namespace serp_metrics {

// static
SerpMetricsServiceFactory* SerpMetricsServiceFactory::GetInstance() {
  static base::NoDestructor<SerpMetricsServiceFactory> instance;
  return instance.get();
}

// static
SerpMetricsService* SerpMetricsServiceFactory::GetFor(
    content::BrowserContext* context) {
  CHECK(context);
  return static_cast<SerpMetricsService*>(
      GetInstance()->GetServiceForContext(context, true));
}

SerpMetricsServiceFactory::SerpMetricsServiceFactory()
    : ProfileKeyedServiceFactory("SerpMetricsService",
                                 ProfileSelections::BuildForRegularProfile()) {}

SerpMetricsServiceFactory::~SerpMetricsServiceFactory() = default;

bool SerpMetricsServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

std::unique_ptr<KeyedService>
SerpMetricsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  Profile* profile = Profile::FromBrowserContext(context);
  CHECK(profile);
  return std::make_unique<SerpMetricsService>(
      CHECK_DEREF(g_browser_process->local_state()),
      SerpMetricsTimePeriodStoreFactory(
          profile->GetPath(), profile_manager->GetProfileAttributesStorage()));
}

}  // namespace serp_metrics
