/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_PIN_SERVICE_FACTORY_
#define BRAVE_BROWSER_IPFS_IPFS_PIN_SERVICE_FACTORY_

#include "base/memory/singleton.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/ipfs/pin/ipfs_remote_pin_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace ipfs {

class IpfsRemotePinServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static IPFSRemotePinService* GetServiceForContext(
      content::BrowserContext* context);
  static IpfsRemotePinServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<IpfsRemotePinServiceFactory>;

  IpfsRemotePinServiceFactory();
  ~IpfsRemotePinServiceFactory() override;

  IpfsRemotePinServiceFactory(const IpfsRemotePinServiceFactory&) = delete;
  IpfsRemotePinServiceFactory& operator=(const IpfsRemotePinServiceFactory&) =
      delete;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_PIN_SERVICE_FACTORY_
