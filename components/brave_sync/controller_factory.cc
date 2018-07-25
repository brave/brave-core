/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/controller_factory.h"

#include "base/memory/singleton.h"
#include "brave/components/brave_sync/controller.h"
#include "brave/components/brave_sync/controller_impl.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"


namespace brave_sync {

// static
Controller* ControllerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<Controller*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
Controller* ControllerFactory::GetForBrowserContextIfExists(
    content::BrowserContext* context) {
  return static_cast<Controller*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

// static
ControllerFactory* ControllerFactory::GetInstance() {
  return base::Singleton<ControllerFactory>::get();
}

ControllerFactory::ControllerFactory()
    : BrowserContextKeyedServiceFactory(
        "BraveSyncController",
        BrowserContextDependencyManager::GetInstance()) {
  ////DependsOn();
}

ControllerFactory::~ControllerFactory() = default;

KeyedService* ControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  ControllerImpl *brave_sync_controller = new brave_sync::ControllerImpl(profile);
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

void ControllerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  // move here brave_sync::prefs::Prefs::RegisterProfilePrefs
}

content::BrowserContext* ControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool ControllerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}


} // namespace brave_sync
