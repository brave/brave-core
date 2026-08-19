// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_settings_service_factory.h"

#include <memory>

#include "base/check_deref.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_origin/brave_origin_service_factory.h"
#include "brave/components/psst/core/browser/pref_names.h"
#include "brave/components/psst/core/browser/psst_settings_service.h"
#include "brave/components/psst/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

// static
PsstSettingsServiceFactory* PsstSettingsServiceFactory::GetInstance() {
  static base::NoDestructor<PsstSettingsServiceFactory> instance;
  return instance.get();
}

// static
psst::PsstSettingsService* PsstSettingsServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<psst::PsstSettingsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

PsstSettingsServiceFactory::PsstSettingsServiceFactory()
    : ProfileKeyedServiceFactory(
          "PsstSettingsService",
          ProfileSelections::Builder()
              // this should match HostContentSettingsMapFactory
              .WithRegular(ProfileSelection::kOwnInstance)
              .WithGuest(ProfileSelection::kOwnInstance)
              .Build()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
  DependsOn(brave_origin::BraveOriginServiceFactory::GetInstance());
}

PsstSettingsServiceFactory::~PsstSettingsServiceFactory() = default;

std::unique_ptr<KeyedService>
PsstSettingsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);

  if (!profile || !psst::features::IsPsstEnabledForProfile(*profile->GetPrefs())) {
    return nullptr;
  }

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
//   auto* brave_origin_service =
//       brave_origin::BraveOriginServiceFactory::GetForProfile(profile);

  return std::make_unique<psst::PsstSettingsService>(
      CHECK_DEREF(map), 
//      brave_origin_service,
      profile->GetPrefs());
}
