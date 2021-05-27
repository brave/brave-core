/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_wallet {
class KeyringController;
class EthJsonRpcController;
}  // namespace brave_wallet

class BraveWalletService : public KeyedService,
                           public base::SupportsWeakPtr<BraveWalletService> {
 public:
  explicit BraveWalletService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveWalletService() override;

  brave_wallet::EthJsonRpcController* rpc_controller() const;
  brave_wallet::KeyringController* keyring_controller() const;

 private:
  std::unique_ptr<brave_wallet::EthJsonRpcController> rpc_controller_;
  std::unique_ptr<brave_wallet::KeyringController> keyring_controller_;

  DISALLOW_COPY_AND_ASSIGN(BraveWalletService);
};

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
