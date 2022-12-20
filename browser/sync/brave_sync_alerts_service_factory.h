/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SYNC_BRAVE_SYNC_ALERTS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SYNC_BRAVE_SYNC_ALERTS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class BraveSyncAlertsService;

class BraveSyncAlertsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  BraveSyncAlertsServiceFactory(const BraveSyncAlertsServiceFactory&) = delete;
  BraveSyncAlertsServiceFactory& operator=(
      const BraveSyncAlertsServiceFactory&) = delete;

  static BraveSyncAlertsService* GetForBrowserContext(
      content::BrowserContext* context);

  static BraveSyncAlertsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BraveSyncAlertsServiceFactory>;

  BraveSyncAlertsServiceFactory();
  ~BraveSyncAlertsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

#endif  // BRAVE_BROWSER_SYNC_BRAVE_SYNC_ALERTS_SERVICE_FACTORY_H_
