// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ipfs/ipfs_local_pin_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"

namespace ipfs {

// static
IpfsLocalPinServiceFactory* IpfsLocalPinServiceFactory::GetInstance() {
  static base::NoDestructor<IpfsLocalPinServiceFactory> instance;
  return instance.get();
}

// static
IpfsLocalPinService* IpfsLocalPinServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!ipfs::IpfsServiceFactory::IsIpfsEnabled(context)) {
    return nullptr;
  }
  return static_cast<IpfsLocalPinService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

IpfsLocalPinServiceFactory::IpfsLocalPinServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "IpfsLocalPinService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ipfs::IpfsServiceFactory::GetInstance());
}

IpfsLocalPinServiceFactory::~IpfsLocalPinServiceFactory() = default;

KeyedService* IpfsLocalPinServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new IpfsLocalPinService(user_prefs::UserPrefs::Get(context),
                                 IpfsServiceFactory::GetForContext(context));
}

content::BrowserContext* IpfsLocalPinServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace ipfs
