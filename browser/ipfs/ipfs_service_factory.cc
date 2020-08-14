/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_factory.h"

#include "brave/browser/ipfs/ipfs_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace ipfs {

// static
IpfsServiceFactory* IpfsServiceFactory::GetInstance() {
  return base::Singleton<IpfsServiceFactory>::get();
}

// static
IpfsService* IpfsServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<IpfsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
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
  return new IpfsService(context);
}

content::BrowserContext* IpfsServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace ipfs
