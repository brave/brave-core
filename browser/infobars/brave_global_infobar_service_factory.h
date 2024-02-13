/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_INFOBAR_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_INFOBAR_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class BraveGlobalInfobarService;

class BraveGlobalInfobarServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static BraveGlobalInfobarServiceFactory* GetInstance();
  static BraveGlobalInfobarService* GetForBrowserContext(
      content::BrowserContext* context);

  BraveGlobalInfobarServiceFactory(const BraveGlobalInfobarServiceFactory&) =
      delete;
  BraveGlobalInfobarServiceFactory& operator=(
      const BraveGlobalInfobarServiceFactory&) = delete;

 private:
  friend base::NoDestructor<BraveGlobalInfobarServiceFactory>;
  BraveGlobalInfobarServiceFactory();
  ~BraveGlobalInfobarServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_INFOBAR_SERVICE_FACTORY_H_
