/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_TODAY_BRAVE_TODAY_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_TODAY_BRAVE_TODAY_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_today {

class BraveTodayService;

class BraveTodayServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static BraveTodayService* GetForProfile(Profile* profile);
  static BraveTodayServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BraveTodayServiceFactory>;

  BraveTodayServiceFactory();
  ~BraveTodayServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(BraveTodayServiceFactory);
};

}  // namespace brave_today

#endif  // BRAVE_BROWSER_BRAVE_TODAY_BRAVE_TODAY_SERVICE_FACTORY_H_
