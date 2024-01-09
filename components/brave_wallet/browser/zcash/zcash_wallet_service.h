/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/zcash/protos/zcash_grpc_data.pb.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_wallet {

class CreateTransparentTransactionTask;
class GetTransparentUtxosContext;

class ZCashWalletService : public KeyedService,
                           public mojom::ZCashWalletService,
                           KeyringServiceObserverBase {
 public:
  using UtxoMap = std::map<std::string, std::vector<zcash::ZCashUtxo>>;
  using RunDiscoveryResult =
      base::expected<std::vector<mojom::ZCashAddressPtr>, std::string>;
  using GetUtxosCallback =
      base::OnceCallback<void(base::expected<UtxoMap, std::string>)>;
  using CreateTransactionCallback =
      base::OnceCallback<void(base::expected<ZCashTransaction, std::string>)>;
  using GetTransactionStatusCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;
  using SignAndPostTransactionCallback =
      base::OnceCallback<void(std::string, ZCashTransaction, std::string)>;
  using RunDiscoveryCallback = base::OnceCallback<void(RunDiscoveryResult)>;
  using DiscoverNextUnusedAddressCallback = base::OnceCallback<void(
      base::expected<mojom::ZCashAddressPtr, std::string>)>;

  ZCashWalletService(
      KeyringService* keyring_service,
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  ZCashWalletService(KeyringService* keyring_service,
                     std::unique_ptr<ZCashRpc> zcash_rpc);

  ~ZCashWalletService() override;

  mojo::PendingRemote<mojom::ZCashWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::ZCashWalletService> receiver);

  void GetBalance(const std::string& chain_id,
                  mojom::AccountIdPtr account_id,
                  GetBalanceCallback) override;

  void GetZCashAccountInfo(mojom::AccountIdPtr account_id,
                           GetZCashAccountInfoCallback callback) override;

  /**
   * Used for internal transfers between own accounts
   */
  void GetReceiverAddress(mojom::AccountIdPtr account_id,
                          GetReceiverAddressCallback callback) override;

  void RunDiscovery(mojom::AccountIdPtr account_id,
                    RunDiscoveryCallback callback);

  void DiscoverNextUnusedAddress(const mojom::AccountIdPtr& account_id,
                                 bool change,
                                 DiscoverNextUnusedAddressCallback callback);

  void GetUtxos(const std::string& chain_id,
                mojom::AccountIdPtr account_id,
                GetUtxosCallback);

  void CreateTransaction(const std::string& chain_id,
                         mojom::AccountIdPtr account_id,
                         const std::string& address_to,
                         uint64_t amount,
                         CreateTransactionCallback callback);

  void GetTransactionStatus(const std::string& chain_id,
                            const std::string& tx_hash,
                            GetTransactionStatusCallback callback);

  void SignAndPostTransaction(const std::string& chain_id,
                              const mojom::AccountIdPtr& account_id,
                              ZCashTransaction zcash_transaction,
                              SignAndPostTransactionCallback callback);

 private:
  friend class ZCashWalletServiceUnitTest;
  friend class CreateTransparentTransactionTask;
  friend class DiscoverNextUnusedZCashAddressTask;
  friend class ZCashTxManager;

  /*KeyringServiceObserverBase*/
  void Unlocked() override;

  bool SignTransactionInternal(ZCashTransaction& tx,
                               const mojom::AccountIdPtr& account_id);

  void AccumulateRunDiscovery(
      base::OnceCallback<
          void(base::expected<mojom::ZCashAddressPtr, std::string>)> result,
      base::expected<mojom::ZCashAddressPtr, std::string> data);
  void OnRunDiscoveryDone(
      mojom::AccountIdPtr account_id,
      RunDiscoveryCallback callback,
      std::vector<base::expected<mojom::ZCashAddressPtr, std::string>> result);
  void OnGetUtxos(
      scoped_refptr<GetTransparentUtxosContext> context,
      const std::string& current_address,
      base::expected<std::vector<zcash::ZCashUtxo>, std::string> result);
  void WorkOnGetUtxos(scoped_refptr<GetTransparentUtxosContext> context);

  void OnDiscoveryDoneForBalance(mojom::AccountIdPtr account_id,
                                 std::string chain_id,
                                 GetBalanceCallback callback,
                                 RunDiscoveryResult discovery_result);
  void OnUtxosResolvedForBalance(GetBalanceCallback initial_callback,
                                 base::expected<UtxoMap, std::string> result);
  void OnTransactionResolvedForStatus(
      GetTransactionStatusCallback callback,
      base::expected<zcash::RawTransaction, std::string> result);

  void OnResolveLastBlockHeightForSendTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& account_id,
      ZCashTransaction zcash_transaction,
      SignAndPostTransactionCallback callback,
      base::expected<zcash::BlockID, std::string> result);

  void OnSendTransactionResult(
      SignAndPostTransactionCallback callback,
      ZCashTransaction zcash_transaction,
      base::expected<zcash::SendResponse, std::string> result);

  void CreateTransactionTaskDone(CreateTransparentTransactionTask* task);

  void UpdateNextUnusedAddressForAccount(const mojom::AccountIdPtr& account_id,
                                         const mojom::ZCashAddressPtr& address);

  ZCashRpc* zcash_rpc();

  raw_ptr<KeyringService> keyring_service_;
  std::unique_ptr<ZCashRpc> zcash_rpc_;

  std::list<std::unique_ptr<CreateTransparentTransactionTask>>
      create_transaction_tasks_;
  mojo::ReceiverSet<mojom::ZCashWalletService> receivers_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  base::WeakPtrFactory<ZCashWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_
