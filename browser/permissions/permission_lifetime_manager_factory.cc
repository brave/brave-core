/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/permission_lifetime_manager_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/permissions/permission_origin_lifetime_monitor_impl.h"
#include "brave/components/permissions/permission_lifetime_manager.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/permissions/features.h"
#include "net/base/features.h"

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
  static base::NoDestructor<PermissionLifetimeManagerFactory> instance;
  return instance.get();
}

PermissionLifetimeManagerFactory::PermissionLifetimeManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "PermissionLifetimeManagerFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(EphemeralStorageServiceFactory::GetInstance());
}

PermissionLifetimeManagerFactory::~PermissionLifetimeManagerFactory() = default;

std::unique_ptr<KeyedService>
PermissionLifetimeManagerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(
          permissions::features::kPermissionLifetime)) {
    return nullptr;
  }
  std::unique_ptr<permissions::PermissionOriginLifetimeMonitor>
      permission_origin_lifetime_monitor;
  if (base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage)) {
    permission_origin_lifetime_monitor =
        std::make_unique<permissions::PermissionOriginLifetimeMonitorImpl>(
            context);
  }
  auto* profile = Profile::FromBrowserContext(context);
  // The HostContentSettingsMap might be null for some irregular profiles, e.g.
  // the System Profile.
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!host_content_settings_map) {
    return nullptr;
  }
  return std::make_unique<permissions::PermissionLifetimeManager>(
      *host_content_settings_map,
      profile->IsOffTheRecord() ? nullptr : profile->GetPrefs(),
      std::move(permission_origin_lifetime_monitor));
}

bool PermissionLifetimeManagerFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

content::BrowserContext*
PermissionLifetimeManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextOwnInstanceInIncognito(context);
}

void PermissionLifetimeManagerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  permissions::PermissionLifetimeManager::RegisterProfilePrefs(registry);
}
