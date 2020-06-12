/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_factory.h"

#include "brave/components/ipfs/browser/ipfs_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
IpfsServiceFactory* IpfsServiceFactory::GetInstance() {
  return base::Singleton<IpfsServiceFactory>::get();
}

// static
IpfsService* IpfsServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<IpfsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

IpfsServiceFactory::IpfsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "IpfsService",
          BrowserContextDependencyManager::GetInstance()) {
}

IpfsServiceFactory::~IpfsServiceFactory() {
}

KeyedService* IpfsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new IpfsService(Profile::FromBrowserContext(context));
}

content::BrowserContext* IpfsServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
