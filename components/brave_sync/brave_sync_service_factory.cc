/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_service_factory.h"

#include "base/memory/singleton.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/brave_sync_service_impl.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"


namespace brave_sync {

// static
BraveSyncService* BraveSyncServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BraveSyncService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BraveSyncService* BraveSyncServiceFactory::GetForBrowserContextIfExists(
    content::BrowserContext* context) {
  return static_cast<BraveSyncService*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

// static
BraveSyncServiceFactory* BraveSyncServiceFactory::GetInstance() {
  return base::Singleton<BraveSyncServiceFactory>::get();
}

BraveSyncServiceFactory::BraveSyncServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "BraveSyncBraveSyncService",
        BrowserContextDependencyManager::GetInstance()) {
  ////DependsOn();
}

BraveSyncServiceFactory::~BraveSyncServiceFactory() = default;

KeyedService* BraveSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  BraveSyncServiceImpl *brave_sync_controller = new brave_sync::BraveSyncServiceImpl(profile);
  //brave_sync_controller->SetProfile(profile);
  // ^ now passed to CTOR

  return brave_sync_controller;

  // BookmarkModel* bookmark_model =
  //     new BookmarkModel(std::make_unique<ChromeBookmarkClient>(
  //         profile, ManagedBookmarkServiceFactory::GetForProfile(profile),
  //         BookmarkSyncServiceFactory::GetForProfile(profile)));
  // bookmark_model->Load(profile->GetPrefs(), profile->GetPath(),
  //                      StartupTaskRunnerServiceFactory::GetForProfile(profile)
  //                          ->GetBookmarkTaskRunner(),
  //                      content::BrowserThread::GetTaskRunnerForThread(
  //                          content::BrowserThread::UI));
  // BookmarkUndoServiceFactory::GetForProfile(profile)->Start(bookmark_model);
  //
  // return bookmark_model;
}

void BraveSyncServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  // move here brave_sync::prefs::Prefs::RegisterProfilePrefs
}

content::BrowserContext* BraveSyncServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool BraveSyncServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}


} // namespace brave_sync
