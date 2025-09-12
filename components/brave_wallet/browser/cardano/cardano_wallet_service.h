/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_

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
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

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
      base::expected<cardano_rpc::UnspentOutputs, std::string>)>;
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

  using GetTransactionStatusCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;
  void GetTransactionStatus(const std::string& chain_id,
                            const std::string& txid,
                            GetTransactionStatusCallback callback);

  std::vector<mojom::CardanoAddressPtr> GetUsedAddresses(
      const mojom::AccountIdPtr& account_id);
  std::vector<mojom::CardanoAddressPtr> GetUnusedAddresses(
      const mojom::AccountIdPtr& account_id);
  mojom::CardanoAddressPtr GetChangeAddress(
      const mojom::AccountIdPtr& account_id);

  KeyringService& keyring_service() { return *keyring_service_; }
  NetworkManager& network_manager() { return *network_manager_; }

  cardano_rpc::CardanoRpc* GetCardanoRpc(const std::string& chain_id);

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  template <typename T>
  using TaskContainer = absl::flat_hash_set<std::unique_ptr<T>>;

  const raw_ref<KeyringService> keyring_service_;
  const raw_ref<NetworkManager> network_manager_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  void OnGetUtxosForGetBalance(
      GetBalanceCallback callback,
      base::expected<cardano_rpc::UnspentOutputs, std::string> utxos);

  void OnGetUtxosTaskDone(
      GetCardanoUtxosTask* task,
      GetUtxosCallback callback,
      base::expected<cardano_rpc::UnspentOutputs, std::string> result);

  void OnCreateCardanoTransactionTaskDone(
      CardanoCreateTransactionTask* task,
      CardanoCreateTransactionTaskCallback callback,
      base::expected<CardanoTransaction, std::string> result);

  void OnPostTransaction(CardanoTransaction bitcoin_transaction,
                         SignAndPostTransactionCallback callback,
                         base::expected<std::string, std::string> txid);

  bool SignTransactionInternal(CardanoTransaction& tx,
                               const mojom::AccountIdPtr& account_id);

  void OnGetTransactionStatus(
      const std::string& txid,
      GetTransactionStatusCallback callback,
      base::expected<std::optional<cardano_rpc::Transaction>, std::string>
          transaction);

  mojo::ReceiverSet<mojom::CardanoWalletService> receivers_;
  cardano_rpc::CardanoRpc cardano_mainnet_rpc_;
  cardano_rpc::CardanoRpc cardano_testnet_rpc_;

  TaskContainer<GetCardanoUtxosTask> get_cardano_utxo_tasks_;
  TaskContainer<CardanoCreateTransactionTask> create_transaction_tasks_;

  base::WeakPtrFactory<CardanoWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_WALLET_SERVICE_H_
