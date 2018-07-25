/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/client_factory.h"

#include "base/memory/singleton.h"
#include "brave/components/brave_sync/client/client_ext_impl.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_sync {

// static
BraveSyncClient* BraveSyncClientFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BraveSyncClient*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BraveSyncClient* BraveSyncClientFactory::GetForBrowserContextIfExists(
    content::BrowserContext* context) {
  return static_cast<BraveSyncClient*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

// static
BraveSyncClientFactory* BraveSyncClientFactory::GetInstance() {
  return base::Singleton<BraveSyncClientFactory>::get();
}

BraveSyncClientFactory::BraveSyncClientFactory()
    : BrowserContextKeyedServiceFactory(
        "BraveSyncClient",
        BrowserContextDependencyManager::GetInstance()) {
  ////DependsOn();
}

BraveSyncClientFactory::~BraveSyncClientFactory() = default;

KeyedService* BraveSyncClientFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  BraveSyncClient *brave_sync_client = new brave_sync::ClientExtImpl(profile);

  return brave_sync_client;
}

void BraveSyncClientFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
}

content::BrowserContext* BraveSyncClientFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool BraveSyncClientFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

} // namespace brave_sync
