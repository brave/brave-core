/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/vpn_service_factory.h"

#include "brave/components/brave_vpn/browser/vpn_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

// static
VpnServiceFactory* VpnServiceFactory::GetInstance() {
  return base::Singleton<VpnServiceFactory>::get();
}

// static
VpnService* VpnServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<VpnService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

VpnServiceFactory::VpnServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "VpnService",
          BrowserContextDependencyManager::GetInstance()) {}

VpnServiceFactory::~VpnServiceFactory() {}

KeyedService* VpnServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new VpnService(Profile::FromBrowserContext(context));
}

content::BrowserContext* VpnServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
