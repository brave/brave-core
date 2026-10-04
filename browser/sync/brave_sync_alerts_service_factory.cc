/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_sync_alerts_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/sync/brave_sync_alerts_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

// static
BraveSyncAlertsService* BraveSyncAlertsServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BraveSyncAlertsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BraveSyncAlertsServiceFactory* BraveSyncAlertsServiceFactory::GetInstance() {
  static base::NoDestructor<BraveSyncAlertsServiceFactory> instance;
  return instance.get();
}

BraveSyncAlertsServiceFactory::BraveSyncAlertsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveSyncAlertsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SyncServiceFactory::GetInstance());
}

BraveSyncAlertsServiceFactory::~BraveSyncAlertsServiceFactory() {}

std::unique_ptr<KeyedService>
BraveSyncAlertsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveSyncAlertsService>(
      Profile::FromBrowserContext(context));
}

content::BrowserContext* BraveSyncAlertsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

bool BraveSyncAlertsServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

bool BraveSyncAlertsServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
