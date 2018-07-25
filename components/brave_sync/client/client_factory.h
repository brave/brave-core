/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace brave_sync {

class BraveSyncClient;

class BraveSyncClientFactory : public BrowserContextKeyedServiceFactory {
public:
  static BraveSyncClient* GetForBrowserContext(
      content::BrowserContext* browser_context);

  static BraveSyncClient* GetForBrowserContextIfExists(
      content::BrowserContext* browser_context);

  static BraveSyncClientFactory* GetInstance();

private:
  friend struct base::DefaultSingletonTraits<BraveSyncClientFactory>;

  BraveSyncClientFactory();
  ~BraveSyncClientFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveSyncClientFactory);
};

} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_FACTORY_H_
