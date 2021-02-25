/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/permission_lifetime_manager_factory.h"

#include "brave/components/permissions/permission_lifetime_manager.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/permissions/features.h"

// static
permissions::PermissionLifetimeManager*
PermissionLifetimeManagerFactory::GetForProfile(
    content::BrowserContext* profile) {
  return static_cast<permissions::PermissionLifetimeManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
PermissionLifetimeManagerFactory*
PermissionLifetimeManagerFactory::GetInstance() {
  return base::Singleton<PermissionLifetimeManagerFactory>::get();
}

PermissionLifetimeManagerFactory::PermissionLifetimeManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "PermissionLifetimeManagerFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(PermissionManagerFactory::GetInstance());
  DependsOn(HostContentSettingsMapFactory::GetInstance());
}

PermissionLifetimeManagerFactory::~PermissionLifetimeManagerFactory() {}

KeyedService* PermissionLifetimeManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(
          permissions::features::kPermissionLifetime)) {
    return nullptr;
  }
  auto* profile = Profile::FromBrowserContext(context);
  return new permissions::PermissionLifetimeManager(
      HostContentSettingsMapFactory::GetForProfile(context),
      profile->IsOffTheRecord() ? nullptr : profile->GetPrefs());
}

bool PermissionLifetimeManagerFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

content::BrowserContext*
PermissionLifetimeManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

void PermissionLifetimeManagerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  permissions::PermissionLifetimeManager::RegisterProfilePrefs(registry);
}
