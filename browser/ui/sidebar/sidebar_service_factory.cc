/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"

#include "base/feature_list.h"
#include "brave/components/sidebar/features.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace sidebar {

// static
SidebarServiceFactory* SidebarServiceFactory::GetInstance() {
  return base::Singleton<SidebarServiceFactory>::get();
}

SidebarService* SidebarServiceFactory::GetForProfile(Profile* profile) {
  if (!base::FeatureList::IsEnabled(kSidebarFeature))
    return nullptr;

  if (profile->IsOffTheRecord()) {
    // Temporarily disable sidebar except normal window.
    return nullptr;
  }

  return static_cast<SidebarService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

SidebarServiceFactory::SidebarServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SidebarService",
          BrowserContextDependencyManager::GetInstance()) {}

SidebarServiceFactory::~SidebarServiceFactory() = default;

KeyedService* SidebarServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SidebarService(Profile::FromBrowserContext(context)->GetPrefs());
}

}  // namespace sidebar
