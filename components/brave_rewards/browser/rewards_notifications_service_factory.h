/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_FACTORY_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_FACTORY_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_rewards {

class RewardsNotificationsService;

class RewardsNotificationsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static brave_rewards::RewardsNotificationsService* GetForProfile(
      Profile* profile);

  static RewardsNotificationsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<RewardsNotificationsServiceFactory>;

  RewardsNotificationsServiceFactory();
  ~RewardsNotificationsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(RewardsNotificationsServiceFactory);
};

}  // namepsace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_FACTORY_
