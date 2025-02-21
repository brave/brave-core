/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/contextual_cueing/contextual_cueing_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/keyed_service.h"

// ContextualCueingService is used by Glic and uses
// PageContentExtractionService both of which we disable, so don't create this
// service.

namespace contextual_cueing {

// static
ContextualCueingService* ContextualCueingServiceFactory::GetForProfile(
    Profile* profile) {
  return nullptr;
}

// static
ContextualCueingServiceFactory* ContextualCueingServiceFactory::GetInstance() {
  static base::NoDestructor<ContextualCueingServiceFactory> instance;
  return instance.get();
}

ContextualCueingServiceFactory::ContextualCueingServiceFactory()
    : ProfileKeyedServiceFactory("ContextualCueingService",
                                 ProfileSelections::BuildNoProfilesSelected()) {
}

ContextualCueingServiceFactory::~ContextualCueingServiceFactory() = default;

std::unique_ptr<KeyedService>
ContextualCueingServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}

bool ContextualCueingServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool ContextualCueingServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace contextual_cueing
