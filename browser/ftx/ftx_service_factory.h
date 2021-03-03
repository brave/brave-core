/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_FTX_FTX_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_FTX_FTX_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class FTXService;
class Profile;

class FTXServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static FTXService* GetForProfile(Profile* profile);
  static FTXServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<FTXServiceFactory>;

  FTXServiceFactory();
  ~FTXServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(FTXServiceFactory);
};

#endif  // BRAVE_BROWSER_FTX_FTX_SERVICE_FACTORY_H_
