/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"

// static
SharedPinnedTabService* SharedPinnedTabServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<SharedPinnedTabService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

SharedPinnedTabServiceFactory* SharedPinnedTabServiceFactory::GetInstance() {
  static base::NoDestructor<SharedPinnedTabServiceFactory> instance;
  return instance.get();
}

SharedPinnedTabServiceFactory::SharedPinnedTabServiceFactory()
    : ProfileKeyedServiceFactory(
          "SharedPinnedTabService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .WithGuest(ProfileSelection::kOwnInstance)
              .Build()) {}

SharedPinnedTabServiceFactory::~SharedPinnedTabServiceFactory() {}

std::unique_ptr<KeyedService>
SharedPinnedTabServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<SharedPinnedTabService>(
      Profile::FromBrowserContext(context));
}

bool SharedPinnedTabServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}
