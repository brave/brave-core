/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_

#include <list>
#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_get_utxos_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"
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

  cardano_rpc::CardanoRpc& cardano_rpc() { return cardano_rpc_; }
  KeyringService& keyring_service() { return *keyring_service_; }
  NetworkManager& network_manager() { return *network_manager_; }

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  const raw_ref<KeyringService> keyring_service_;
  const raw_ref<NetworkManager> network_manager_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  void OnGetCardanoUtxosTaskDone(
      GetCardanoUtxosTask* task,
      base::expected<GetCardanoUtxosTask::UtxoMap, std::string> result);

  mojo::ReceiverSet<mojom::CardanoWalletService> receivers_;
  cardano_rpc::CardanoRpc cardano_rpc_;

  std::list<std::pair<std::unique_ptr<GetCardanoUtxosTask>, GetBalanceCallback>>
      get_cardano_utxo_tasks_;

  base::WeakPtrFactory<CardanoWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
