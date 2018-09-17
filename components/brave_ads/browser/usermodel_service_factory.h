/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_USERMODEL_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_USERMODEL_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_ads {
class UsermodelService;

// Singleton that owns all UsermodelService and associates them with
// Profiles.
class UsermodelServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static brave_ads::UsermodelService* GetForProfile(Profile* profile);

  static UsermodelServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<UsermodelServiceFactory>;

  UsermodelServiceFactory();
  ~UsermodelServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(UsermodelServiceFactory);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_USERMODEL_SERVICE_FACTORY_H_
