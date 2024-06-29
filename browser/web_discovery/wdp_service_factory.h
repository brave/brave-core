/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEB_DISCOVERY_WDP_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_WEB_DISCOVERY_WDP_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace web_discovery {

class WDPService;

class WDPServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static WDPServiceFactory* GetInstance();
  static WDPService* GetForBrowserContext(content::BrowserContext* context);

 private:
  friend base::NoDestructor<WDPServiceFactory>;

  WDPServiceFactory();
  ~WDPServiceFactory() override;

  WDPServiceFactory(const WDPServiceFactory&) = delete;
  WDPServiceFactory& operator=(const WDPServiceFactory&) = delete;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace web_discovery

#endif  // BRAVE_BROWSER_WEB_DISCOVERY_WDP_SERVICE_FACTORY_H_
