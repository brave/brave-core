/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_ads {
class AdsService;

// Singleton that owns all AdsService and associates them with Profiles.
class AdsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static brave_ads::AdsService* GetForProfile(Profile* profile);

  static AdsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AdsServiceFactory>;

  AdsServiceFactory();
  ~AdsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ShouldMigratePrefs(user_prefs::PrefRegistrySyncable* registry) const;
  bool ShouldMigratePrefsFrom62(
      user_prefs::PrefRegistrySyncable* registry) const;

  DISALLOW_COPY_AND_ASSIGN(AdsServiceFactory);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_FACTORY_H_
