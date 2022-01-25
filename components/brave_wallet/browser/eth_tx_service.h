/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_block_tracker.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_wallet {

class EthTxServiceUnitTest;

class AssetRatioService;
class JsonRpcService;
class KeyringService;

class EthTxService : public KeyedService,
                     public mojom::EthTxService,
                     public mojom::KeyringServiceObserver,
                     public EthBlockTracker::Observer,
                     public EthTxStateManager::Observer {
 public:
  explicit EthTxService(JsonRpcService* json_rpc_service,
                        KeyringService* keyring_service,
                        AssetRatioService* asset_ratio_service,
                        std::unique_ptr<EthTxStateManager> tx_state_manager,
                        std::unique_ptr<EthNonceTracker> nonce_tracker,
                        std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
                        PrefService* prefs);
  ~EthTxService() override;
  EthTxService(const EthTxService&) = delete;
  EthTxService operator=(const EthTxService&) = delete;

  mojo::PendingRemote<mojom::EthTxService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::EthTxService> receiver);

  void AddUnapprovedTransaction(mojom::TxDataPtr tx_data,
                                const std::string& from,
                                AddUnapprovedTransactionCallback) override;
  void AddUnapproved1559Transaction(
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      AddUnapproved1559TransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void RejectTransaction(const std::string& tx_meta_id,
                         RejectTransactionCallback) override;
  void MakeERC20TransferData(const std::string& to_address,
                             const std::string& amount,
                             MakeERC20TransferDataCallback) override;
  void MakeERC20ApproveData(const std::string& to_address,
                            const std::string& amount,
                            MakeERC20ApproveDataCallback) override;

  void MakeERC721TransferFromData(const std::string& from,
                                  const std::string& to,
                                  const std::string& token_id,
                                  const std::string& contract_address,
                                  MakeERC721TransferFromDataCallback) override;

  void GetAllTransactionInfo(const std::string& from,
                             GetAllTransactionInfoCallback) override;

  void SetGasPriceAndLimitForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::string& gas_price,
      const std::string& gas_limit,
      SetGasPriceAndLimitForUnapprovedTransactionCallback callback) override;
  void SetGasFeeAndLimitForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::string& max_priority_fee_per_gas,
      const std::string& max_fee_per_gas,
      const std::string& gas_limit,
      SetGasFeeAndLimitForUnapprovedTransactionCallback callback) override;
  void SetDataForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::vector<uint8_t>& data,
      SetDataForUnapprovedTransactionCallback callback) override;
  void SetNonceForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::string& nonce,
      SetNonceForUnapprovedTransactionCallback) override;
  void GetNonceForHardwareTransaction(
      const std::string& tx_meta_id,
      GetNonceForHardwareTransactionCallback callback) override;
  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;
  void ProcessHardwareSignature(
      const std::string& tx_meta_id,
      const std::string& v,
      const std::string& r,
      const std::string& s,
      ProcessHardwareSignatureCallback callback) override;
  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;
  void AddObserver(
      ::mojo::PendingRemote<mojom::EthTxServiceObserver> observer) override;
  // Resets things back to the original state of EthTxService
  // To be used when the Wallet is reset / erased
  void Reset() override;

  static bool ValidateTxData(const mojom::TxDataPtr& tx_data,
                             std::string* error);
  static bool ValidateTxData1559(const mojom::TxData1559Ptr& tx_data,
                                 std::string* error);
  std::unique_ptr<EthTxStateManager::TxMeta> GetTxForTesting(
      const std::string& tx_meta_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthTxServiceUnitTest, TestSubmittedToConfirmed);
  FRIEND_TEST_ALL_PREFIXES(EthTxServiceUnitTest, RetryTransaction);
  FRIEND_TEST_ALL_PREFIXES(EthTxServiceUnitTest, Reset);
  friend class EthTxServiceUnitTest;

  void NotifyUnapprovedTxUpdated(EthTxStateManager::TxMeta* meta);
  void OnConnectionError();
  void OnGetNextNonce(std::unique_ptr<EthTxStateManager::TxMeta> meta,
                      uint256_t chain_id,
                      ApproveTransactionCallback callback,
                      bool success,
                      uint256_t nonce);
  void OnGetNextNonceForHardware(
      std::unique_ptr<EthTxStateManager::TxMeta> meta,
      GetNonceForHardwareTransactionCallback callback,
      bool success,
      uint256_t nonce);
  void PublishTransaction(const std::string& tx_meta_id,
                          const std::string& signed_transaction,
                          ApproveTransactionCallback callback);
  void OnPublishTransaction(std::string tx_meta_id,
                            ApproveTransactionCallback callback,
                            const std::string& tx_hash,
                            mojom::ProviderError error,
                            const std::string& error_message);
  void OnGetGasPrice(const std::string& from,
                     const std::string& to,
                     const std::string& value,
                     const std::string& data,
                     const std::string& gas_limit,
                     std::unique_ptr<EthTransaction> tx,
                     AddUnapprovedTransactionCallback callback,
                     const std::string& result,
                     mojom::ProviderError error,
                     const std::string& error_message);
  void ContinueAddUnapprovedTransaction(
      const std::string& from,
      std::unique_ptr<EthTransaction> tx,
      AddUnapprovedTransactionCallback callback,
      const std::string& result,
      mojom::ProviderError error,
      const std::string& error_message);
  void OnGetGasOracle(const std::string& from,
                      const std::string& to,
                      const std::string& value,
                      const std::string& data,
                      const std::string& gas_limit,
                      std::unique_ptr<Eip1559Transaction> tx,
                      AddUnapprovedTransactionCallback callback,
                      mojom::GasEstimation1559Ptr gas_estimation);
  void CheckIfBlockTrackerShouldRun();
  void UpdatePendingTransactions();

  void ContinueSpeedupOrCancelTransaction(
      const std::string& from,
      const std::string& gas_limit,
      std::unique_ptr<EthTransaction> tx,
      SpeedupOrCancelTransactionCallback callback,
      const std::string& result,
      mojom::ProviderError error,
      const std::string& error_message);
  void ContinueSpeedupOrCancel1559Transaction(
      const std::string& from,
      const std::string& gas_limit,
      std::unique_ptr<Eip1559Transaction> tx,
      SpeedupOrCancelTransactionCallback callback,
      mojom::GasEstimation1559Ptr gas_estimation);

  void ContinueMakeERC721TransferFromData(
      const std::string& from,
      const std::string& to,
      uint256_t token_id,
      MakeERC721TransferFromDataCallback callback,
      bool is_safe_transfer_from_supported,
      mojom::ProviderError error,
      const std::string& error_message);

  // KeyringServiceObserver:
  void KeyringCreated() override;
  void KeyringRestored() override;
  void KeyringReset() override;
  void Locked() override;
  void Unlocked() override;
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged() override {}

  // EthBlockTracker::Observer:
  void OnLatestBlock(uint256_t block_num) override {}
  void OnNewBlock(uint256_t block_num) override;

  // EthTxStateManager::Observer
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override;

  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;        // NOT OWNED
  raw_ptr<KeyringService> keyring_service_ = nullptr;         // NOT OWNED
  raw_ptr<AssetRatioService> asset_ratio_service_ = nullptr;  // NOT OWNED
  raw_ptr<PrefService> prefs_ = nullptr;                      // NOT OWNED
  std::unique_ptr<EthTxStateManager> tx_state_manager_;
  std::unique_ptr<EthNonceTracker> nonce_tracker_;
  std::unique_ptr<EthPendingTxTracker> pending_tx_tracker_;
  std::unique_ptr<EthBlockTracker> eth_block_tracker_;
  bool known_no_pending_tx = false;

  mojo::RemoteSet<mojom::EthTxServiceObserver> observers_;
  mojo::ReceiverSet<mojom::EthTxService> receivers_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};

  base::WeakPtrFactory<EthTxService> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_SERVICE_H_
