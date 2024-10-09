/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_service_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_service.h"

// static
ReadAnythingService* ReadAnythingServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return nullptr;
}

// static
ReadAnythingServiceFactory* ReadAnythingServiceFactory::GetInstance() {
  static base::NoDestructor<ReadAnythingServiceFactory> instance;
  return instance.get();
}

ReadAnythingServiceFactory::ReadAnythingServiceFactory()
    : ProfileKeyedServiceFactory(
          "ReadAnythingServiceFactory",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .Build()) {}

bool ReadAnythingServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return false;
}

std::unique_ptr<KeyedService>
ReadAnythingServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}
