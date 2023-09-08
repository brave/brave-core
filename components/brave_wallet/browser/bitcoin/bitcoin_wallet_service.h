/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {
class GetBalanceContext;
class GetUtxosContext;
class CreateTransactionTask;

class BitcoinWalletService : public KeyedService,
                             public mojom::BitcoinWalletService,
                             KeyringServiceObserverBase {
 public:
  BitcoinWalletService(
      KeyringService* keyring_service,
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BitcoinWalletService() override;

  mojo::PendingRemote<mojom::BitcoinWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BitcoinWalletService> receiver);

  void GetBalance(const std::string& chain_id,
                  mojom::AccountIdPtr account_id,
                  GetBalanceCallback callback) override;
  void GetBitcoinAccountInfo(const std::string& chain_id,
                             mojom::AccountIdPtr account_id,
                             GetBitcoinAccountInfoCallback callback) override;
  mojom::BitcoinAccountInfoPtr GetBitcoinAccountInfoSync(
      const std::string& chain_id,
      mojom::AccountIdPtr account_id);

  void SendTo(const std::string& chain_id,
              mojom::AccountIdPtr account_id,
              const std::string& address_to,
              uint64_t amount,
              uint64_t fee,
              SendToCallback callback) override;

  using UtxoMap =
      std::map<std::string, std::vector<bitcoin_rpc::UnspentOutput>>;
  using GetUtxosCallback =
      base::OnceCallback<void(base::expected<UtxoMap, std::string>)>;
  void GetUtxos(const std::string& chain_id,
                mojom::AccountIdPtr account_id,
                GetUtxosCallback callback);

  using CreateTransactionCallback =
      base::OnceCallback<void(base::expected<BitcoinTransaction, std::string>)>;
  void CreateTransaction(const std::string& chain_id,
                         mojom::AccountIdPtr account_id,
                         const std::string& address_to,
                         uint64_t amount,
                         CreateTransactionCallback callback);

  using SignAndPostTransactionCallback =
      base::OnceCallback<void(std::string, BitcoinTransaction, std::string)>;
  void SignAndPostTransaction(const std::string& chain_id,
                              const mojom::AccountIdPtr& account_id,
                              BitcoinTransaction bitcoin_transaction,
                              SignAndPostTransactionCallback callback);

  using GetTransactionStatusCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;
  void GetTransactionStatus(const std::string& chain_id,
                            const std::string& txid,
                            GetTransactionStatusCallback callback);

  bitcoin_rpc::BitcoinRpc& bitcoin_rpc() { return *bitcoin_rpc_; }

  absl::optional<std::string> GetUnusedChangeAddress(
      const mojom::AccountId& account_id);

 private:
  friend CreateTransactionTask;

  void OnGetAddressStatsForBalance(
      scoped_refptr<GetBalanceContext> context,
      std::string address,
      base::expected<bitcoin_rpc::AddressStats, std::string> stats);
  void WorkOnGetBalance(scoped_refptr<GetBalanceContext> context);

  void OnGetUtxos(scoped_refptr<GetUtxosContext> context,
                  std::string address,
                  base::expected<std::vector<bitcoin_rpc::UnspentOutput>,
                                 std::string> utxos);
  void WorkOnGetUtxos(scoped_refptr<GetUtxosContext> context);

  void OnPostTransaction(BitcoinTransaction bitcoin_transaction,
                         SignAndPostTransactionCallback callback,
                         base::expected<std::string, std::string> txid);

  void OnGetTransaction(
      const std::string& txid,
      GetTransactionStatusCallback callback,
      base::expected<bitcoin_rpc::Transaction, std::string> transaction);

  bool SignTransactionInternal(BitcoinTransaction& tx,
                               const mojom::AccountIdPtr& account_id);
  void CreateTransactionTaskDone(CreateTransactionTask* task);

  raw_ptr<KeyringService> keyring_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::list<std::unique_ptr<CreateTransactionTask>> create_transaction_tasks_;
  mojo::ReceiverSet<mojom::BitcoinWalletService> receivers_;
  std::unique_ptr<bitcoin_rpc::BitcoinRpc> bitcoin_rpc_;
  base::WeakPtrFactory<BitcoinWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_
