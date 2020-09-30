/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_ads {
class AdsService;

// Singleton that owns all AdsService and associates them with Profiles.
class AdsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  AdsServiceFactory(const AdsServiceFactory&) = delete;
  AdsServiceFactory& operator=(const AdsServiceFactory&) = delete;

  static brave_ads::AdsService* GetForProfile(Profile* profile);

  static AdsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AdsServiceFactory>;

  AdsServiceFactory();
  ~AdsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_FACTORY_H_
