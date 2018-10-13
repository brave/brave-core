/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_service_factory.h"

#include "base/memory/singleton.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/brave_sync_service_impl.h"
#include "brave/components/brave_sync/client/brave_sync_client_factory.h"
#include "brave/components/brave_sync/pref_names.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace brave_sync {

// static
BraveSyncService* BraveSyncServiceFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return GetForProfile(Profile::FromBrowserContext(browser_context));
}

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
  DependsOn(BraveSyncClientFactory::GetInstance());
}

BraveSyncServiceFactory::~BraveSyncServiceFactory() = default;

KeyedService* BraveSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  std::unique_ptr<BraveSyncServiceImpl> brave_sync_service(
      new BraveSyncServiceImpl(Profile::FromBrowserContext(context)));
  return brave_sync_service.release();
}

void BraveSyncServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(prefs::kThisDeviceId, std::string());
  registry->RegisterStringPref(prefs::kSeed, std::string());
  registry->RegisterStringPref(prefs::kThisDeviceName, std::string());
  registry->RegisterStringPref(prefs::kBookmarksBaseOrder, std::string());

  registry->RegisterBooleanPref(prefs::kSyncThisDeviceEnabled, false);
  registry->RegisterBooleanPref(prefs::kSyncBookmarksEnabled, false);
  registry->RegisterBooleanPref(prefs::kSiteSettingsEnabled, false);
  registry->RegisterBooleanPref(prefs::kHistoryEnabled, false);

  registry->RegisterTimePref(prefs::kLatestRecordTime, base::Time());
  registry->RegisterTimePref(prefs::kLastFetchTime, base::Time());
}

content::BrowserContext* BraveSyncServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool BraveSyncServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace brave_sync
