/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_IPFS_IPFS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;
namespace ipfs {
class IpfsService;

class IpfsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  IpfsServiceFactory(const IpfsServiceFactory&) = delete;
  IpfsServiceFactory& operator=(const IpfsServiceFactory&) = delete;

  static IpfsService* GetForContext(content::BrowserContext* context);
  static IpfsServiceFactory* GetInstance();
  static bool IsIpfsEnabled(content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<IpfsServiceFactory>;

  IpfsServiceFactory();
  ~IpfsServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_SERVICE_FACTORY_H_
