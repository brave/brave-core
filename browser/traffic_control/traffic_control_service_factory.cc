// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_service_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/traffic_control/core/browser/prefs_registration.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"

// static
TrafficControlServiceFactory* TrafficControlServiceFactory::GetInstance() {
  static base::NoDestructor<TrafficControlServiceFactory> instance;
  return instance.get();
}

// static
traffic_control::TrafficControlService*
TrafficControlServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<traffic_control::TrafficControlService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

TrafficControlServiceFactory::TrafficControlServiceFactory()
    : ProfileKeyedServiceFactory("TrafficControlService",
                                 ProfileSelections::BuildForRegularProfile()) {}

TrafficControlServiceFactory::~TrafficControlServiceFactory() = default;

void TrafficControlServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  traffic_control::RegisterProfilePrefs(registry);
}

bool TrafficControlServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

std::unique_ptr<KeyedService>
TrafficControlServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(
          traffic_control::features::kTrafficControl)) {
    return nullptr;
  }

  auto* profile = Profile::FromBrowserContext(context);
  return std::make_unique<traffic_control::TrafficControlService>(
      profile->GetPrefs());
}
