/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browsing_topics/browsing_topics_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/keyed_service.h"

// Brave disables BrowsingTopics by stubbing out the service factory.
// This ensures BrowsingTopicsService is never created regardless of
// feature flag state.

namespace browsing_topics {

// static
BrowsingTopicsService* BrowsingTopicsServiceFactory::GetForProfile(
    Profile* profile) {
  return nullptr;
}

// static
BrowsingTopicsServiceFactory* BrowsingTopicsServiceFactory::GetInstance() {
  static base::NoDestructor<BrowsingTopicsServiceFactory> instance;
  return instance.get();
}

BrowsingTopicsServiceFactory::BrowsingTopicsServiceFactory()
    : ProfileKeyedServiceFactory("BrowsingTopicsService",
                                 ProfileSelections::BuildNoProfilesSelected()) {
}

BrowsingTopicsServiceFactory::~BrowsingTopicsServiceFactory() = default;

std::unique_ptr<KeyedService>
BrowsingTopicsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}

bool BrowsingTopicsServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return false;
}

}  // namespace browsing_topics
