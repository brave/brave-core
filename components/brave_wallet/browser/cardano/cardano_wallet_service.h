/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_wallet {
class KeyringService;
class NetworkManager;

class CardanoWalletService : public mojom::CardanoWalletService {
 public:
  CardanoWalletService(
      KeyringService& keyring_service,
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~CardanoWalletService() override;

  void Bind(mojo::PendingReceiver<mojom::CardanoWalletService> receiver);

  void Reset();

  // mojom::CardanoWalletService:
  void GetBalance(mojom::AccountIdPtr account_id,
                  GetBalanceCallback callback) override;

  KeyringService& keyring_service() { return *keyring_service_; }

 private:
  const raw_ref<KeyringService> keyring_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  mojo::ReceiverSet<mojom::CardanoWalletService> receivers_;
  base::WeakPtrFactory<CardanoWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
