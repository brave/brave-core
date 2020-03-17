/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include "brave/browser/extensions/brave_theme_event_router.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

BraveThemeService::BraveThemeService(Profile* profile) {
  brave_theme_event_router_.reset(
      new extensions::BraveThemeEventRouter(profile));
}

BraveThemeService::~BraveThemeService() = default;

void BraveThemeService::SetBraveThemeEventRouterForTesting(
    extensions::BraveThemeEventRouter* mock_router) {
  brave_theme_event_router_.reset(mock_router);
}

// static
BraveThemeService* BraveThemeServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BraveThemeService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BraveThemeServiceFactory* BraveThemeServiceFactory::GetInstance() {
  return base::Singleton<BraveThemeServiceFactory>::get();
}

BraveThemeServiceFactory::BraveThemeServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveThemeService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveThemeServiceFactory::~BraveThemeServiceFactory() = default;

KeyedService* BraveThemeServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* browser_context) const {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  return new BraveThemeService(profile);
}

bool BraveThemeServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}
