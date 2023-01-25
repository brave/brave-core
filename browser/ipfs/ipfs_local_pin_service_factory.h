// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_IPFS_IPFS_LOCAL_PIN_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_IPFS_IPFS_LOCAL_PIN_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace ipfs {

class IpfsLocalPinService;

class IpfsLocalPinServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static IpfsLocalPinService* GetServiceForContext(
      content::BrowserContext* context);
  static IpfsLocalPinServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<IpfsLocalPinServiceFactory>;

  IpfsLocalPinServiceFactory();
  ~IpfsLocalPinServiceFactory() override;

  IpfsLocalPinServiceFactory(const IpfsLocalPinServiceFactory&) = delete;
  IpfsLocalPinServiceFactory& operator=(const IpfsLocalPinServiceFactory&) =
      delete;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_LOCAL_PIN_SERVICE_FACTORY_H_
