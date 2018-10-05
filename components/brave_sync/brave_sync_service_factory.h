/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_sync {

class BraveSyncService;

class BraveSyncServiceFactory : public BrowserContextKeyedServiceFactory {
public:
  static BraveSyncService* GetForProfile(Profile* profile);
  static BraveSyncService* GetForBrowserContext(
      content::BrowserContext* context);

  static BraveSyncServiceFactory* GetInstance();

private:
  friend struct base::DefaultSingletonTraits<BraveSyncServiceFactory>;

  BraveSyncServiceFactory();
  ~BraveSyncServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveSyncServiceFactory);
};

} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_FACTORY_H_
