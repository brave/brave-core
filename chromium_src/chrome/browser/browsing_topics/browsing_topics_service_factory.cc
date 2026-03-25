/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Brave disables BrowsingTopics by stubbing out the service factory.
// This ensures BrowsingTopicsService is never created regardless of
// feature flag state.

#include "chrome/browser/browsing_topics/browsing_topics_service_factory.h"

// Pre-include headers that also declare BuildServiceInstanceForBrowserContext
// to prevent the #define below from corrupting their declarations.
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"

#define BuildServiceInstanceForBrowserContext \
  BuildServiceInstanceForBrowserContext_ChromiumImpl
#include <chrome/browser/browsing_topics/browsing_topics_service_factory.cc>
#undef BuildServiceInstanceForBrowserContext

namespace browsing_topics {

std::unique_ptr<KeyedService>
BrowsingTopicsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return nullptr;
}

}  // namespace browsing_topics
