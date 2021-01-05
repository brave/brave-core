/* Copyright (c) 2020 The Brave Authors. All rights reserved.
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

  return static_cast<SidebarService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

SidebarServiceFactory::SidebarServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SidebarService",
          BrowserContextDependencyManager::GetInstance()) {}

SidebarServiceFactory::~SidebarServiceFactory() = default;

content::BrowserContext* SidebarServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Normal & OTR profile share the sidebar configuration.
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

KeyedService* SidebarServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SidebarService(Profile::FromBrowserContext(context)->GetPrefs());
}

}  // namespace sidebar
