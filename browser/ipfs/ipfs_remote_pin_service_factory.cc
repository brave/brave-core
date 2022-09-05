/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_remote_pin_service_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/pin/ipfs_remote_pin_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace ipfs {

// static
IpfsRemotePinServiceFactory* IpfsRemotePinServiceFactory::GetInstance() {
  return base::Singleton<IpfsRemotePinServiceFactory>::get();
}

// static
IPFSRemotePinService* IpfsRemotePinServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  //  if (!IsAllowedForContext(context)) {
  //    return nullptr;
  //  }
  return static_cast<IPFSRemotePinService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

IpfsRemotePinServiceFactory::IpfsRemotePinServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "IpfsRemotePinService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ipfs::IpfsServiceFactory::GetInstance());
}

IpfsRemotePinServiceFactory::~IpfsRemotePinServiceFactory() = default;

KeyedService* IpfsRemotePinServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new IPFSRemotePinService(IpfsServiceFactory::GetForContext(context));
}

content::BrowserContext* IpfsRemotePinServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace ipfs
