/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_COOKIE_PREF_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_COOKIE_PREF_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace brave_shields {

class CookiePrefService;

class CookiePrefServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  CookiePrefServiceFactory(const CookiePrefServiceFactory&) = delete;
  CookiePrefServiceFactory& operator=(const CookiePrefServiceFactory&) = delete;

  static CookiePrefService* GetForBrowserContext(
      content::BrowserContext* context);

  static CookiePrefServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<CookiePrefServiceFactory>;

  CookiePrefServiceFactory();
  ~CookiePrefServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;

  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_COOKIE_PREF_SERVICE_FACTORY_H_
