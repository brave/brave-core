/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/ad_block_pref_service_factory.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/ad_block_pref_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_shields {

// static
AdBlockPrefService* AdBlockPrefServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdBlockPrefService*>(
      GetInstance()->GetServiceForBrowserContext(context,
                                                 /*create_service=*/true));
}

// static
AdBlockPrefServiceFactory* AdBlockPrefServiceFactory::GetInstance() {
  return base::Singleton<AdBlockPrefServiceFactory>::get();
}

AdBlockPrefServiceFactory::AdBlockPrefServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdBlockPrefService",
          BrowserContextDependencyManager::GetInstance()) {}

AdBlockPrefServiceFactory::~AdBlockPrefServiceFactory() {}

KeyedService* AdBlockPrefServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AdBlockPrefService(
      g_brave_browser_process->ad_block_service(),
      Profile::FromBrowserContext(context)->GetPrefs());
}

content::BrowserContext* AdBlockPrefServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool AdBlockPrefServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace brave_shields
