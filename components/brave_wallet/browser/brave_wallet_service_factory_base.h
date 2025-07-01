/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_FACTORY_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_FACTORY_BASE_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "brave/components/profile_keyed_service_factory_shim/profile_keyed_service_factory_shim.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_wallet {

class BraveWalletService;
class BraveWalletServiceDelegate;

template <typename PKSF>
class BraveWalletServiceFactoryBase
    : public ProfileKeyedServiceFactoryShim<PKSF> {
 public:
  template <typename... Args>
  explicit BraveWalletServiceFactoryBase(Args&&... args)
      : ProfileKeyedServiceFactoryShim<PKSF>(std::forward<Args>(args)...) {}

 private:
  using Context = ProfileKeyedServiceFactoryShim<PKSF>::Context;

  virtual scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory(
      Context context) const = 0;

  virtual std::unique_ptr<BraveWalletServiceDelegate>
  GetBraveWalletServiceDelegate(Context context) const = 0;

  virtual PrefService* GetProfilePrefs(Context context) const = 0;

  virtual PrefService* GetLocalState() const = 0;

  // ProfileKeyedServiceFactoryShim<PKSF>:
  std::unique_ptr<KeyedService> BuildServiceInstanceForContext(
      Context context) const override {
    return std::make_unique<BraveWalletService>(
        GetURLLoaderFactory(context), GetBraveWalletServiceDelegate(context),
        GetProfilePrefs(context), GetLocalState());
  }
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_FACTORY_BASE_H_
