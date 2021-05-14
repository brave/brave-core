// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

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

  FTXServiceFactory(const FTXServiceFactory&) = delete;
  FTXServiceFactory& operator=(FTXServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_FTX_FTX_SERVICE_FACTORY_H_
