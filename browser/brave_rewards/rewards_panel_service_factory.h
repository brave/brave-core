/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_rewards {

class RewardsPanelService;

// A service factory for |RewardsPanelService|.
class RewardsPanelServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static RewardsPanelService* GetForProfile(Profile* profile);

  static RewardsPanelServiceFactory* GetInstance();

  RewardsPanelServiceFactory(const RewardsPanelServiceFactory&) = delete;
  RewardsPanelServiceFactory& operator=(const RewardsPanelServiceFactory&) =
      delete;

 private:
  friend base::NoDestructor<RewardsPanelServiceFactory>;

  RewardsPanelServiceFactory();
  ~RewardsPanelServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_SERVICE_FACTORY_H_
