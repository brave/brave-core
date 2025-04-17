/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_

#include <list>
#include <memory>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_create_transaction_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_get_utxos_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_wallet {

class KeyringService;
class CardanoTransaction;
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

  using DiscoverNextUnusedAddressCallback = base::OnceCallback<void(
      base::expected<mojom::CardanoAddressPtr, std::string>)>;
  void DiscoverNextUnusedAddress(const mojom::AccountIdPtr& account_id,
                                 mojom::CardanoKeyRole role,
                                 DiscoverNextUnusedAddressCallback callback);

  using GetUtxosCallback = base::OnceCallback<void(
      base::expected<GetCardanoUtxosTask::UtxoMap, std::string>)>;
  void GetUtxos(mojom::AccountIdPtr account_id, GetUtxosCallback callback);

  using CardanoCreateTransactionTaskCallback =
      base::OnceCallback<void(base::expected<CardanoTransaction, std::string>)>;
  void CreateCardanoTransaction(mojom::AccountIdPtr account_id,
                                const CardanoAddress& address_to,
                                uint64_t amount,
                                bool sending_max_amount,
                                CardanoCreateTransactionTaskCallback callback);

  using SignAndPostTransactionCallback =
      base::OnceCallback<void(std::string, CardanoTransaction, std::string)>;
  void SignAndPostTransaction(const mojom::AccountIdPtr& account_id,
                              CardanoTransaction cardano_transaction,
                              SignAndPostTransactionCallback callback);

  cardano_rpc::CardanoRpc& cardano_rpc() { return cardano_rpc_; }
  KeyringService& keyring_service() { return *keyring_service_; }
  NetworkManager& network_manager() { return *network_manager_; }

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  using CardanoGetUtxosTaskList =
      std::list<std::unique_ptr<GetCardanoUtxosTask>>;
  using CardanoCreateTransactionTaskList =
      std::list<std::unique_ptr<CardanoCreateTransactionTask>>;

  const raw_ref<KeyringService> keyring_service_;
  const raw_ref<NetworkManager> network_manager_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  void OnGetUtxosForGetBalance(
      GetBalanceCallback callback,
      base::expected<GetCardanoUtxosTask::UtxoMap, std::string> utxos);

  void OnGetUtxosTaskDone(
      CardanoGetUtxosTaskList::iterator task,
      GetUtxosCallback callback,
      base::expected<GetCardanoUtxosTask::UtxoMap, std::string> result);

  void OnCreateCardanoTransactionTaskDone(
      CardanoCreateTransactionTaskList::iterator task,
      CardanoCreateTransactionTaskCallback callback,
      base::expected<CardanoTransaction, std::string> result);

  void OnPostTransaction(CardanoTransaction bitcoin_transaction,
                         SignAndPostTransactionCallback callback,
                         base::expected<std::string, std::string> txid);

  bool SignTransactionInternal(CardanoTransaction& tx,
                               const mojom::AccountIdPtr& account_id);

  mojo::ReceiverSet<mojom::CardanoWalletService> receivers_;
  cardano_rpc::CardanoRpc cardano_rpc_;

  CardanoGetUtxosTaskList get_cardano_utxo_tasks_;
  CardanoCreateTransactionTaskList create_transaction_tasks_;

  base::WeakPtrFactory<CardanoWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
