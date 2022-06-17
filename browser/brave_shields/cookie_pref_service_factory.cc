/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/cookie_pref_service_factory.h"

#include "brave/components/brave_shields/browser/cookie_pref_service.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_shields {

// static
CookiePrefService* CookiePrefServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<CookiePrefService*>(
      GetInstance()->GetServiceForBrowserContext(context,
                                                 /*create_service=*/true));
}

// static
CookiePrefServiceFactory* CookiePrefServiceFactory::GetInstance() {
  return base::Singleton<CookiePrefServiceFactory>::get();
}

CookiePrefServiceFactory::CookiePrefServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "CookiePrefService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
  DependsOn(CookieSettingsFactory::GetInstance());
}

CookiePrefServiceFactory::~CookiePrefServiceFactory() = default;

KeyedService* CookiePrefServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  return new CookiePrefService(
      HostContentSettingsMapFactory::GetForProfile(profile),
      CookieSettingsFactory::GetForProfile(profile).get(), profile->GetPrefs());
}

bool CookiePrefServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace brave_shields
