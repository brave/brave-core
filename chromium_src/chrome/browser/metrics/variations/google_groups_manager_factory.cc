/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/metrics/variations/google_groups_manager_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/variations/service/google_groups_manager.h"

// static
GoogleGroupsManagerFactory* GoogleGroupsManagerFactory::GetInstance() {
  static base::NoDestructor<GoogleGroupsManagerFactory> instance;
  return instance.get();
}

// static
GoogleGroupsManager* GoogleGroupsManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return nullptr;
}

GoogleGroupsManagerFactory::GoogleGroupsManagerFactory()
    : ProfileKeyedServiceFactory(
          "GoogleGroupsManager",
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
GoogleGroupsManagerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}

bool GoogleGroupsManagerFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool GoogleGroupsManagerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void GoogleGroupsManagerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}
