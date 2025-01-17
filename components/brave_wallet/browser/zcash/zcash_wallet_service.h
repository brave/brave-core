/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ZCASH_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ZCASH_WALLET_SERVICE_H_

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
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"
#endif

namespace brave_wallet {

class OrchardSyncState;
class ZCashCreateOrchardToOrchardTransactionTask;
class ZCashCreateTransparentToOrchardTransactionTask;
class ZCashCreateTransparentTransactionTask;
class ZCashGetTransparentUtxosContext;
class ZCashGetZCashChainTipStatusTask;
class ZCashResolveBalanceTask;

class ZCashWalletService : public mojom::ZCashWalletService,
#if BUILDFLAG(ENABLE_ORCHARD)
                           public ZCashShieldSyncService::Observer,
#endif
                           KeyringServiceObserverBase {
 public:
  using UtxoMap =
      std::map<std::string, std::vector<zcash::mojom::ZCashUtxoPtr>>;
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
      base::FilePath zcash_data_path,
      KeyringService& keyring_service,
      NetworkManager* network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // Constructor for tests
  ZCashWalletService(base::FilePath zcash_data_path,
                     KeyringService& keyring_service,
                     std::unique_ptr<ZCashRpc> zcash_rpc);

  ~ZCashWalletService() override;

  void Bind(mojo::PendingReceiver<mojom::ZCashWalletService> receiver);

  // Returns transparent balance for the account
  void GetBalance(const std::string& chain_id,
                  mojom::AccountIdPtr account_id,
                  GetBalanceCallback) override;

  void GetZCashAccountInfo(mojom::AccountIdPtr account_id,
                           GetZCashAccountInfoCallback callback) override;

  void MakeAccountShielded(mojom::AccountIdPtr account_id,
                           uint32_t account_birthday_block,
                           MakeAccountShieldedCallback callback) override;

  // Starts Orchard pool syncing for the provided account.
  void StartShieldSync(mojom::AccountIdPtr account_id,
                       uint32_t to,
                       StartShieldSyncCallback callback) override;
  void StopShieldSync(mojom::AccountIdPtr account_id,
                      StopShieldSyncCallback callback) override;
  void GetChainTipStatus(mojom::AccountIdPtr account_id,
                         const std::string& chain_id,
                         GetChainTipStatusCallback callback) override;

  /**
   * Used for internal transfers between own accounts
   */
  void GetReceiverAddress(mojom::AccountIdPtr account_id,
                          GetReceiverAddressCallback callback) override;

  // TODO(cypt4): Make this a part of zcash
  // transaction
  // Sends all account funds from transparent pool to orchard pool.
  void ShieldAllFunds(const std::string& chain_id,
                      mojom::AccountIdPtr account_id,
                      ShieldAllFundsCallback callback) override;

  void ResetSyncState(mojom::AccountIdPtr account_id,
                      ResetSyncStateCallback callback) override;

  void RunDiscovery(mojom::AccountIdPtr account_id,
                    RunDiscoveryCallback callback);

  virtual void DiscoverNextUnusedAddress(
      const mojom::AccountIdPtr& account_id,
      bool change,
      DiscoverNextUnusedAddressCallback callback);

  void ValidateZCashAddress(const std::string& addr,
                            bool testnet,
                            ValidateZCashAddressCallback callback) override;

  void AddObserver(
      mojo::PendingRemote<mojom::ZCashWalletServiceObserver> observer) override;

  virtual void GetUtxos(const std::string& chain_id,
                        const mojom::AccountIdPtr& account_id,
                        GetUtxosCallback);

  void CreateFullyTransparentTransaction(const std::string& chain_id,
                                         mojom::AccountIdPtr account_id,
                                         const std::string& address_to,
                                         uint64_t amount,
                                         CreateTransactionCallback callback);

#if BUILDFLAG(ENABLE_ORCHARD)
  void CreateOrchardToOrchardTransaction(const std::string& chain_id,
                                         mojom::AccountIdPtr account_id,
                                         const std::string& address_to,
                                         uint64_t amount,
                                         std::optional<OrchardMemo> memo,
                                         CreateTransactionCallback callback);
  void CreateTransparentToOrchardTransaction(
      const std::string& chain_id,
      mojom::AccountIdPtr account_id,
      const std::string& address_to,
      uint64_t amount,
      std::optional<OrchardMemo> memo,
      CreateTransactionCallback callback);
#endif

  void GetTransactionStatus(const std::string& chain_id,
                            const std::string& tx_hash,
                            GetTransactionStatusCallback callback);

  void CompleteAndPostTransaction(const std::string& chain_id,
                                  ZCashTransaction zcash_transaction,
                                  SignAndPostTransactionCallback callback);

  void SignAndPostTransaction(const std::string& chain_id,
                              const mojom::AccountIdPtr& account_id,
                              const ZCashTransaction& zcash_transaction,
                              SignAndPostTransactionCallback callback);

  void SetZCashRpcForTesting(std::unique_ptr<ZCashRpc> zcash_rpc);

  void Reset();

 private:
  friend class ZCashCompleteTransactionTask;
  friend class ZCashCreateTransparentToOrchardTransactionTask;
  friend class ZCashCreateOrchardToOrchardTransactionTask;
  friend class ZCashCreateTransparentTransactionTask;
  friend class ZCashDiscoverNextUnusedZCashAddressTask;
  friend class ZCashGetZCashChainTipStatusTask;
  friend class ZCashResolveBalanceTask;
  friend class ZCashTxManager;

  friend class ZCashWalletServiceUnitTest;
  friend class ZCashShieldSyncServiceTest;

  /*KeyringServiceObserverBase*/
  void Unlocked() override;
  void Locked() override;

  void AccumulateRunDiscovery(
      base::OnceCallback<
          void(base::expected<mojom::ZCashAddressPtr, std::string>)> result,
      base::expected<mojom::ZCashAddressPtr, std::string> data);
  void OnRunDiscoveryDone(
      mojom::AccountIdPtr account_id,
      RunDiscoveryCallback callback,
      std::vector<base::expected<mojom::ZCashAddressPtr, std::string>> result);
  void OnGetUtxos(scoped_refptr<ZCashGetTransparentUtxosContext> context,
                  const std::string& current_address,
                  base::expected<zcash::mojom::GetAddressUtxosResponsePtr,
                                 std::string> result);
  void WorkOnGetUtxos(scoped_refptr<ZCashGetTransparentUtxosContext> context);

  void OnDiscoveryDoneForBalance(mojom::AccountIdPtr account_id,
                                 std::string chain_id,
                                 GetBalanceCallback callback,
                                 RunDiscoveryResult discovery_result);
  void OnUtxosResolvedForBalance(GetBalanceCallback initial_callback,
                                 base::expected<UtxoMap, std::string> result);

  void OnTransactionResolvedForStatus(
      GetTransactionStatusCallback callback,
      base::expected<zcash::mojom::RawTransactionPtr, std::string> result);

  void OnSendTransactionResult(
      SignAndPostTransactionCallback callback,
      ZCashTransaction zcash_transaction,
      base::expected<zcash::mojom::SendResponsePtr, std::string> result);

  void OnResolveBalanceResult(
      GetBalanceCallback callback,
      base::expected<mojom::ZCashBalancePtr, std::string> result);

  void CreateTransactionTaskDone(ZCashCreateTransparentTransactionTask* task);
  void CompleteTransactionTaskDone(ZCashCompleteTransactionTask* task);
  void ResolveBalanceTaskDone(ZCashResolveBalanceTask* task);
  void CompleteTransactionDone(std::string chain_id,
                               ZCashTransaction original_zcash_transaction,
                               SignAndPostTransactionCallback callback,
                               base::expected<ZCashTransaction, std::string>);

#if BUILDFLAG(ENABLE_ORCHARD)
  virtual void CreateTransactionTaskDone(
      ZCashCreateTransparentToOrchardTransactionTask* task);
  virtual void CreateTransactionTaskDone(
      ZCashCreateOrchardToOrchardTransactionTask* task);
  void GetZCashChainTipStatusTaskDone(ZCashGetZCashChainTipStatusTask* task);
  void OnGetChainTipStatusResult(
      GetChainTipStatusCallback callback,
      base::expected<mojom::ZCashChainTipStatusPtr, std::string> result);

  void CreateShieldAllTransaction(const std::string& chain_id,
                                  mojom::AccountIdPtr account_id,
                                  CreateTransactionCallback callback);
  void CreateShieldAllTransactionTaskDone(
      const std::string& chain_id,
      mojom::AccountIdPtr account_id,
      ShieldAllFundsCallback callback,
      base::expected<ZCashTransaction, std::string> transaction);

  void OnPostShieldTransactionDone(ShieldAllFundsCallback callback,
                                   std::string tx_id,
                                   ZCashTransaction transaction,
                                   std::string error);

  // Methods for retrieving account birthday block
  void GetLatestBlockForAccountBirthday(mojom::AccountIdPtr account_id,
                                        MakeAccountShieldedCallback callback);
  void OnGetLatestBlockForAccountBirthday(
      mojom::AccountIdPtr account_id,
      MakeAccountShieldedCallback callback,
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);
  void GetTreeStateForAccountBirthday(mojom::AccountIdPtr account_id,
                                      uint32_t block_id,
                                      MakeAccountShieldedCallback callback);
  void OnGetTreeStateForAccountBirthday(
      mojom::AccountIdPtr account_id,
      MakeAccountShieldedCallback callback,
      base::expected<zcash::mojom::TreeStatePtr, std::string> result);

  mojom::ZCashAccountShieldBirthdayPtr GetAccountShieldBirthday(
      const mojom::AccountIdPtr& account_id);
  bool SetAccountShieldBirthday(std::string network_id,
                                mojom::AccountIdPtr account_id);

  std::vector<mojom::AccountIdPtr> GetShieldedAccounts();

  void OnSyncStart(const mojom::AccountIdPtr& account_id) override;
  void OnSyncStop(const mojom::AccountIdPtr& account_id) override;
  void OnSyncError(const mojom::AccountIdPtr& account_id,
                   const std::string& error) override;
  void OnSyncStatusUpdate(
      const mojom::AccountIdPtr& account_id,
      const mojom::ZCashShieldSyncStatusPtr& status) override;

  void OnResetSyncState(
      ResetSyncStateCallback callback,
      base::expected<OrchardStorage::Result, OrchardStorage::Error> result);

  base::SequenceBound<OrchardSyncState>& sync_state();

  void OverrideSyncStateForTesting(
      base::SequenceBound<OrchardSyncState> sync_state);
#endif

  ZCashActionContext CreateActionContext(const mojom::AccountIdPtr& account_id,
                                         const std::string chain_id);

  void UpdateNextUnusedAddressForAccount(const mojom::AccountIdPtr& account_id,
                                         const mojom::ZCashAddressPtr& address);

  ZCashRpc& zcash_rpc();
  KeyringService& keyring_service();

  base::FilePath zcash_data_path_;
  raw_ref<KeyringService> keyring_service_;
  std::unique_ptr<ZCashRpc> zcash_rpc_;

  std::list<std::unique_ptr<ZCashCompleteTransactionTask>>
      complete_transaction_tasks_;
  std::list<std::unique_ptr<ZCashCreateTransparentTransactionTask>>
      create_transaction_tasks_;
  std::list<std::unique_ptr<ZCashResolveBalanceTask>> resolve_balance_tasks_;

#if BUILDFLAG(ENABLE_ORCHARD)
  base::SequenceBound<OrchardSyncState> sync_state_;
  std::list<std::unique_ptr<ZCashCreateTransparentToOrchardTransactionTask>>
      create_shield_transaction_tasks_;
  std::list<std::unique_ptr<ZCashCreateOrchardToOrchardTransactionTask>>
      create_shielded_transaction_tasks_;
  std::map<mojom::AccountIdPtr, std::unique_ptr<ZCashShieldSyncService>>
      shield_sync_services_;
  std::list<std::unique_ptr<ZCashGetZCashChainTipStatusTask>>
      get_zcash_chain_tip_status_tasks_;
#endif

  mojo::RemoteSet<mojom::ZCashWalletServiceObserver> observers_;
  mojo::ReceiverSet<mojom::ZCashWalletService> receivers_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  base::WeakPtrFactory<ZCashWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ZCASH_WALLET_SERVICE_H_
