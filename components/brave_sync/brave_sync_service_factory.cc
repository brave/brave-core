/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_service_factory.h"

#include <memory>
#include <string>

#include "base/memory/singleton.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/components/brave_sync/brave_sync_service_impl.h"
#endif

namespace brave_sync {

// static
BraveSyncService* BraveSyncServiceFactory::GetForProfile(Profile* profile) {
  if (profile->IsOffTheRecord())
    return NULL;

  return static_cast<BraveSyncService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BraveSyncServiceFactory* BraveSyncServiceFactory::GetInstance() {
  return base::Singleton<BraveSyncServiceFactory>::get();
}

BraveSyncServiceFactory::BraveSyncServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "BraveSyncService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(BookmarkModelFactory::GetInstance());
}

BraveSyncServiceFactory::~BraveSyncServiceFactory() = default;

KeyedService* BraveSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  std::unique_ptr<BraveSyncServiceImpl> brave_sync_service(
      new BraveSyncServiceImpl(Profile::FromBrowserContext(context)));
  return brave_sync_service.release();
#else
  return nullptr;
#endif
}

void BraveSyncServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(prefs::kSyncDeviceId, std::string());
  registry->RegisterStringPref(prefs::kSyncSeed, std::string());
  registry->RegisterStringPref(prefs::kSyncPrevSeed, std::string());
  registry->RegisterStringPref(prefs::kSyncDeviceName, std::string());
  registry->RegisterStringPref(prefs::kSyncBookmarksBaseOrder, std::string());

  registry->RegisterBooleanPref(prefs::kSyncEnabled, false);
  registry->RegisterBooleanPref(prefs::kSyncBookmarksEnabled, false);
  registry->RegisterBooleanPref(prefs::kSyncSiteSettingsEnabled, false);
  registry->RegisterBooleanPref(prefs::kSyncHistoryEnabled, false);

  registry->RegisterTimePref(prefs::kSyncLatestRecordTime, base::Time());
  registry->RegisterTimePref(prefs::kSyncLastFetchTime, base::Time());

  registry->RegisterStringPref(prefs::kSyncDeviceList, std::string());
  registry->RegisterStringPref(prefs::kSyncApiVersion, std::string("0"));
  registry->RegisterIntegerPref(prefs::kSyncMigrateBookmarksVersion, 0);
}

content::BrowserContext* BraveSyncServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool BraveSyncServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace brave_sync
