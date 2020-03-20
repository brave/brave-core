/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/binance/binance_service_factory.h"

#include "brave/components/binance/browser/binance_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
BinanceServiceFactory* BinanceServiceFactory::GetInstance() {
  return base::Singleton<BinanceServiceFactory>::get();
}

// static
BinanceService* BinanceServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BinanceService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

BinanceServiceFactory::BinanceServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BinanceService",
          BrowserContextDependencyManager::GetInstance()) {
}

BinanceServiceFactory::~BinanceServiceFactory() {
}

KeyedService* BinanceServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BinanceService(Profile::FromBrowserContext(context));
}

content::BrowserContext* BinanceServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
