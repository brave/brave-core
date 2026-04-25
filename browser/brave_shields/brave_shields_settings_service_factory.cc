// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "content/public/browser/browser_context.h"

// static
BraveShieldsSettingsServiceFactory*
BraveShieldsSettingsServiceFactory::GetInstance() {
  static base::NoDestructor<BraveShieldsSettingsServiceFactory> instance;
  return instance.get();
}

// static
brave_shields::BraveShieldsSettingsService*
BraveShieldsSettingsServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<brave_shields::BraveShieldsSettingsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

BraveShieldsSettingsServiceFactory::BraveShieldsSettingsServiceFactory()
    : ProfileKeyedServiceFactory(
          "BraveShieldsSettingsService",
          ProfileSelections::Builder()
              // this should match HostContentSettingsMapFactory
              .WithRegular(ProfileSelection::kOwnInstance)
              .WithGuest(ProfileSelection::kOwnInstance)
              .Build()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
}

BraveShieldsSettingsServiceFactory::~BraveShieldsSettingsServiceFactory() =
    default;

std::unique_ptr<KeyedService>
BraveShieldsSettingsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  CHECK(map);
  return std::make_unique<brave_shields::BraveShieldsSettingsService>(
      *map, g_browser_process->local_state(), profile->GetPrefs());
}
