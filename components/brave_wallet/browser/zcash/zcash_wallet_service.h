/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_

#include <list>
#include <map>
#include <memory>
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
  using GetUtxosCallback =
      base::OnceCallback<void(base::expected<UtxoMap, std::string>)>;
  using CreateTransactionCallback =
      base::OnceCallback<void(base::expected<ZCashTransaction, std::string>)>;
  using GetTransactionStatusCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;
  using SignAndPostTransactionCallback =
      base::OnceCallback<void(std::string, ZCashTransaction, std::string)>;

  ZCashWalletService(KeyringService* keyring_service, ZCashRpc* zcash_rpc);
  ~ZCashWalletService() override;

  mojo::PendingRemote<mojom::ZCashWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::ZCashWalletService> receiver);

  void GetBalance(const std::string& chain_id,
                  mojom::AccountIdPtr account_id,
                  GetBalanceCallback) override;

  /**
   * Used for internal transfers between own accounts
   */
  void GetReceiverAddress(mojom::AccountIdPtr account_id,
                          GetReceiverAddressCallback callback) override;

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
  friend class CreateTransparentTransactionTask;
  friend class ZCashTxManager;

  absl::optional<std::string> GetUnusedChangeAddress(
      const mojom::AccountId& account_id);

  bool SignTransactionInternal(ZCashTransaction& tx,
                               const mojom::AccountIdPtr& account_id);

  void OnGetUtxos(
      scoped_refptr<GetTransparentUtxosContext> context,
      const std::string& current_address,
      base::expected<std::vector<zcash::ZCashUtxo>, std::string> result);
  void WorkOnGetUtxos(scoped_refptr<GetTransparentUtxosContext> context);

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

  ZCashRpc* zcash_rpc();

  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<ZCashRpc> zcash_rpc_;

  std::list<std::unique_ptr<CreateTransparentTransactionTask>>
      create_transaction_tasks_;
  mojo::ReceiverSet<mojom::ZCashWalletService> receivers_;
  base::WeakPtrFactory<ZCashWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_
