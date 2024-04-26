/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/wdp_service_factory.h"

#include "brave/components/web_discovery/browser/wdp_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace web_discovery {

WDPServiceFactory* WDPServiceFactory::GetInstance() {
  static base::NoDestructor<WDPServiceFactory> instance;
  return instance.get();
}

WDPServiceFactory::WDPServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WDPService",
          BrowserContextDependencyManager::GetInstance()) {}

WDPServiceFactory::~WDPServiceFactory() = default;

KeyedService* WDPServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new WDPService(user_prefs::UserPrefs::Get(context));
}

content::BrowserContext* WDPServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Prevents creation of service instance for incognito/OTR profiles
  return context->IsOffTheRecord() ? nullptr : context;
}

bool WDPServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace web_discovery
