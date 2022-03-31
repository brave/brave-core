/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "net/base/features.h"

// static
EphemeralStorageServiceFactory* EphemeralStorageServiceFactory::GetInstance() {
  return base::Singleton<EphemeralStorageServiceFactory>::get();
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
}

EphemeralStorageServiceFactory::~EphemeralStorageServiceFactory() {}

KeyedService* EphemeralStorageServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage)) {
    return nullptr;
  }
  return new ephemeral_storage::EphemeralStorageService(
      context, HostContentSettingsMapFactory::GetForProfile(context));
}

content::BrowserContext* EphemeralStorageServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
