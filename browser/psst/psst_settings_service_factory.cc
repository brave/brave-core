// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_settings_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/psst/browser/core/psst_settings_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
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
}

PsstSettingsServiceFactory::~PsstSettingsServiceFactory() = default;

std::unique_ptr<KeyedService>
PsstSettingsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  CHECK(map);
  return std::make_unique<psst::PsstSettingsService>(*map);
}
