/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/metrics/variations/google_groups_updater_service_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/variations/service/google_groups_updater_service.h"

// static
GoogleGroupsUpdaterServiceFactory*
GoogleGroupsUpdaterServiceFactory::GetInstance() {
  return nullptr;
}

// static
GoogleGroupsUpdaterService*
GoogleGroupsUpdaterServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return nullptr;
}

GoogleGroupsUpdaterServiceFactory::GoogleGroupsUpdaterServiceFactory()
    : ProfileKeyedServiceFactory(
          "GoogleGroupsUpdaterService",
          // We only want instances of this service corresponding to regular
          // profiles, as those are the only ones that can have sync data to
          // copy from.
          // In the case of Incognito, the OTR profile will not have the service
          // created however the owning regular profile will be loaded and have
          // the service created.
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .WithGuest(ProfileSelection::kNone)
              .WithSystem(ProfileSelection::kNone)
              .WithAshInternals(ProfileSelection::kNone)
              .Build()) {}

std::unique_ptr<KeyedService>
GoogleGroupsUpdaterServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}

bool GoogleGroupsUpdaterServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool GoogleGroupsUpdaterServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void GoogleGroupsUpdaterServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}
