/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_FACTORY_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_FACTORY_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class BraveNavigationUIData;
class Profile;

namespace tor {
class TorProfileService;
}  // namespace tor

// Singleton that owns all TorProfileService and associates them with
// Profiles.
class TorProfileServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static tor::TorProfileService* GetForProfile(Profile* profile);

  static TorProfileServiceFactory* GetInstance();

  static void SetTorNavigationUIData(Profile* profile,
                                     BraveNavigationUIData* data);

 private:
  friend struct base::DefaultSingletonTraits<TorProfileServiceFactory>;

  TorProfileServiceFactory();
  ~TorProfileServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  void BrowserContextShutdown(content::BrowserContext* context) override;
  void BrowserContextDestroyed(content::BrowserContext* context) override;

  DISALLOW_COPY_AND_ASSIGN(TorProfileServiceFactory);
};

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_FACTORY_
