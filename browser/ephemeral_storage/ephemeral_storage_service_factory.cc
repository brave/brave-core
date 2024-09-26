/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/ephemeral_storage/brave_ephemeral_storage_service_delegate.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_pref_names.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "net/base/features.h"

// static
EphemeralStorageServiceFactory* EphemeralStorageServiceFactory::GetInstance() {
  static base::NoDestructor<EphemeralStorageServiceFactory> instance;
  return instance.get();
}

// static
ephemeral_storage::EphemeralStorageService*
EphemeralStorageServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<ephemeral_storage::EphemeralStorageService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

EphemeralStorageServiceFactory::EphemeralStorageServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "EphemeralStorageService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
  DependsOn(CookieSettingsFactory::GetInstance());
}

EphemeralStorageServiceFactory::~EphemeralStorageServiceFactory() = default;

void EphemeralStorageServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(
      ephemeral_storage::kFirstPartyStorageOriginsToCleanup);
}

std::unique_ptr<KeyedService>
EphemeralStorageServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage) &&
      !base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage) &&
      !base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage)) {
    return nullptr;
  }
  // The HostContentSettingsMap might be null for some irregular profiles, e.g.
  // the System Profile.
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(context);
  if (!host_content_settings_map) {
    return nullptr;
  }
  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<ephemeral_storage::EphemeralStorageService>(
      context, host_content_settings_map,
      std::make_unique<ephemeral_storage::BraveEphemeralStorageServiceDelegate>(
          context, host_content_settings_map,
          CookieSettingsFactory::GetForProfile(profile)));
}

content::BrowserContext* EphemeralStorageServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextOwnInstanceInIncognito(context);
}

bool EphemeralStorageServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}
