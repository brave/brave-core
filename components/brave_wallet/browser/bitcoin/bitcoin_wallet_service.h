/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_discover_account_task.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_wallet {
class CreateTransactionTask;
class GetUtxosTask;
class GetBalanceTask;
class DiscoverNextUnusedAddressTask;
class FetchRawTransactionsTask;
class KeyringService;
class NetworkManager;

class BitcoinWalletService : public mojom::BitcoinWalletService,
                             public KeyringServiceObserverBase {
 public:
  BitcoinWalletService(
      KeyringService& keyring_service,
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BitcoinWalletService() override;

  void Bind(mojo::PendingReceiver<mojom::BitcoinWalletService> receiver);

  void Reset();

  // mojom::BitcoinWalletService:
  void GetBalance(mojom::AccountIdPtr account_id,
                  GetBalanceCallback callback) override;
  void GetExtendedKeyAccountBalance(
      const std::string& chain_id,
      const std::string& extended_key,
      GetExtendedKeyAccountBalanceCallback callback) override;
  void GetBitcoinAccountInfo(mojom::AccountIdPtr account_id,
                             GetBitcoinAccountInfoCallback callback) override;
  mojom::BitcoinAccountInfoPtr GetBitcoinAccountInfoSync(
      const mojom::AccountIdPtr& account_id);
  void RunDiscovery(mojom::AccountIdPtr account_id,
                    bool change,
                    RunDiscoveryCallback callback) override;

  // KeyringServiceObserverBase:
  void AccountsAdded(std::vector<mojom::AccountInfoPtr> accounts) override;

  // address -> related utxo list
  using UtxoMap = std::map<std::string, bitcoin_rpc::UnspentOutputs>;
  using GetUtxosCallback =
      base::OnceCallback<void(base::expected<UtxoMap, std::string>)>;
  void GetUtxos(mojom::AccountIdPtr account_id, GetUtxosCallback callback);

  using CreateTransactionCallback =
      base::OnceCallback<void(base::expected<BitcoinTransaction, std::string>)>;
  void CreateTransaction(mojom::AccountIdPtr account_id,
                         const std::string& address_to,
                         uint64_t amount,
                         bool sending_max_amount,
                         CreateTransactionCallback callback);

  using SignAndPostTransactionCallback =
      base::OnceCallback<void(std::string, BitcoinTransaction, std::string)>;
  void SignAndPostTransaction(const mojom::AccountIdPtr& account_id,
                              BitcoinTransaction bitcoin_transaction,
                              SignAndPostTransactionCallback callback);

  using PostHwSignedTransactionCallback =
      base::OnceCallback<void(std::string, BitcoinTransaction, std::string)>;
  void PostHwSignedTransaction(const mojom::AccountIdPtr& account_id,
                               BitcoinTransaction bitcoin_transaction,
                               const mojom::BitcoinSignature& hw_signature,
                               PostHwSignedTransactionCallback callback);

  using GetTransactionStatusCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;
  void GetTransactionStatus(const std::string& chain_id,
                            const std::string& txid,
                            GetTransactionStatusCallback callback);

  using FetchRawTransactionsCallback = base::OnceCallback<void(
      base::expected<std::vector<std::vector<uint8_t>>, std::string>)>;
  void FetchRawTransactions(const std::string& network_id,
                            const std::vector<SHA256HashArray>& txids,
                            FetchRawTransactionsCallback callback);

  using DiscoverNextUnusedAddressCallback = base::OnceCallback<void(
      base::expected<mojom::BitcoinAddressPtr, std::string>)>;
  void DiscoverNextUnusedAddress(const mojom::AccountIdPtr& account_id,
                                 bool change,
                                 DiscoverNextUnusedAddressCallback callback);

  using DiscoverWalletAccountCallback =
      DiscoverWalletAccountTask::DiscoverAccountCallback;
  void DiscoverWalletAccount(mojom::KeyringId keyring_id,
                             uint32_t account_index,
                             DiscoverWalletAccountCallback callback);

  mojom::BtcHardwareTransactionSignDataPtr GetBtcHardwareTransactionSignData(
      const BitcoinTransaction& tx,
      const mojom::AccountIdPtr& account_id);

  bitcoin_rpc::BitcoinRpc& bitcoin_rpc() { return bitcoin_rpc_; }
  KeyringService& keyring_service() { return *keyring_service_; }

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  void SetArrangeTransactionsForTesting(bool arrange);

 private:
  template <typename T>
  using TaskContainer = absl::flat_hash_set<std::unique_ptr<T>>;

  friend CreateTransactionTask;

  void OnRunDiscoveryDone(
      mojom::AccountIdPtr account_id,
      RunDiscoveryCallback callback,
      base::expected<mojom::BitcoinAddressPtr, std::string>);
  void UpdateNextUnusedAddressForAccount(
      const mojom::AccountIdPtr& account_id,
      const mojom::BitcoinAddressPtr& address);

  void OnPostTransaction(BitcoinTransaction bitcoin_transaction,
                         SignAndPostTransactionCallback callback,
                         base::expected<std::string, std::string> txid);

  void OnGetTransaction(
      const std::string& txid,
      GetTransactionStatusCallback callback,
      base::expected<bitcoin_rpc::Transaction, std::string> transaction);

  void OnDiscoverWalletAccountDone(
      DiscoverWalletAccountTask* task,
      DiscoverWalletAccountCallback callback,
      base::expected<DiscoveredBitcoinAccount, std::string> result);
  void OnGetExtendedKeyAccountBalanceDone(
      DiscoverExtendedKeyAccountTask* task,
      GetExtendedKeyAccountBalanceCallback callback,
      base::expected<DiscoveredBitcoinAccount, std::string> result);

  void OnAddedAccountDiscoveryDone(
      DiscoverWalletAccountTask* task,
      mojom::AccountIdPtr account_id,
      base::expected<DiscoveredBitcoinAccount, std::string> result);

  void OnFetchRawTransactionsDone(
      FetchRawTransactionsTask* task,
      FetchRawTransactionsCallback callback,
      base::expected<std::vector<std::vector<uint8_t>>, std::string> result);

  void OnDiscoverNextUnusedAddressDone(
      DiscoverNextUnusedAddressTask* task,
      DiscoverNextUnusedAddressCallback callback,
      base::expected<mojom::BitcoinAddressPtr, std::string> result);

  void OnGetUtxosTaskDone(GetUtxosTask* task,
                          GetUtxosCallback callback,
                          base::expected<UtxoMap, std::string> result);

  void OnGetBalanceTaskDone(
      GetBalanceTask* task,
      GetBalanceCallback callback,
      base::expected<mojom::BitcoinBalancePtr, std::string> balance);

  bool SignTransactionInternal(BitcoinTransaction& tx,
                               const mojom::AccountIdPtr& account_id);
  bool ApplyHwSignatureInternal(BitcoinTransaction& tx,
                                const mojom::BitcoinSignature& hw_signature);
  void OnCreateTransactionTaskDone(
      CreateTransactionTask* task,
      CreateTransactionCallback callback,
      base::expected<BitcoinTransaction, std::string> result);

  const raw_ref<KeyringService> keyring_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  TaskContainer<CreateTransactionTask> create_transaction_tasks_;
  TaskContainer<GetUtxosTask> get_utxos_tasks_;
  TaskContainer<DiscoverWalletAccountTask> discover_wallet_account_tasks_;
  TaskContainer<DiscoverNextUnusedAddressTask>
      discover_next_unused_address_tasks_;
  TaskContainer<GetBalanceTask> get_balance_tasks_;
  TaskContainer<DiscoverExtendedKeyAccountTask>
      discover_extended_key_account_tasks_;
  TaskContainer<FetchRawTransactionsTask> fetch_raw_transactions_tasks_;

  mojo::ReceiverSet<mojom::BitcoinWalletService> receivers_;
  bitcoin_rpc::BitcoinRpc bitcoin_rpc_;
  bool arrange_transactions_for_testing_ = false;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};
  base::WeakPtrFactory<BitcoinWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_WALLET_SERVICE_H_
