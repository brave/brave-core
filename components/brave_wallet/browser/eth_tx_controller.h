/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

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

class EthTxControllerUnitTest;

class AssetRatioController;
class EthJsonRpcController;
class KeyringController;

class EthTxController : public KeyedService,
                        public mojom::EthTxController,
                        public mojom::KeyringControllerObserver,
                        public EthBlockTracker::Observer,
                        public EthTxStateManager::Observer {
 public:
  explicit EthTxController(
      EthJsonRpcController* eth_json_rpc_controller,
      KeyringController* keyring_controller,
      AssetRatioController* asset_ratio_controller,
      std::unique_ptr<EthTxStateManager> tx_state_manager,
      std::unique_ptr<EthNonceTracker> nonce_tracker,
      std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
      PrefService* prefs);
  ~EthTxController() override;
  EthTxController(const EthTxController&) = delete;
  EthTxController operator=(const EthTxController&) = delete;

  mojo::PendingRemote<mojom::EthTxController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::EthTxController> receiver);

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
  void ApproveHardwareTransaction(
      const std::string& tx_meta_id,
      ApproveHardwareTransactionCallback callback) override;
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
  void GetTransactionInfo(const std::string& tx_meta_id,
                          GetTransactionInfoCallback callback) override;
  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;
  void AddObserver(
      ::mojo::PendingRemote<mojom::EthTxControllerObserver> observer) override;

  static bool ValidateTxData(const mojom::TxDataPtr& tx_data,
                             std::string* error);
  static bool ValidateTxData1559(const mojom::TxData1559Ptr& tx_data,
                                 std::string* error);
  std::unique_ptr<EthTxStateManager::TxMeta> GetTxForTesting(
      const std::string& tx_meta_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthTxControllerUnitTest, TestSubmittedToConfirmed);
  FRIEND_TEST_ALL_PREFIXES(EthTxControllerUnitTest, RetryTransaction);
  friend class EthTxControllerUnitTest;

  void NotifyUnapprovedTxUpdated(EthTxStateManager::TxMeta* meta);
  void OnConnectionError();
  void OnGetNextNonce(std::unique_ptr<EthTxStateManager::TxMeta> meta,
                      uint256_t chain_id,
                      bool success,
                      uint256_t nonce);
  void OnGetNextNonceForHardware(
      std::unique_ptr<EthTxStateManager::TxMeta> meta,
      ApproveHardwareTransactionCallback callback,
      bool success,
      uint256_t nonce);
  void PublishTransaction(const std::string& tx_meta_id,
                          const std::string& signed_transaction);
  void OnPublishTransaction(std::string tx_meta_id,
                            bool status,
                            const std::string& tx_hash);
  void OnGetGasPrice(const std::string& from,
                     const std::string& to,
                     const std::string& value,
                     const std::string& data,
                     const std::string& gas_limit,
                     std::unique_ptr<EthTransaction> tx,
                     AddUnapprovedTransactionCallback callback,
                     bool success,
                     const std::string& result);
  void ContinueAddUnapprovedTransaction(
      const std::string& from,
      std::unique_ptr<EthTransaction> tx,
      AddUnapprovedTransactionCallback callback,
      bool success,
      const std::string& result);
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
      bool success,
      const std::string& result);
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
      bool success,
      bool is_safe_transfer_from_supported);

  // KeyringControllerObserver:
  void KeyringCreated() override;
  void KeyringRestored() override;
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

  EthJsonRpcController* rpc_controller_;          // NOT OWNED
  KeyringController* keyring_controller_;         // NOT OWNED
  AssetRatioController* asset_ratio_controller_;  // NOT OWNED
  std::unique_ptr<EthTxStateManager> tx_state_manager_;
  std::unique_ptr<EthNonceTracker> nonce_tracker_;
  std::unique_ptr<EthPendingTxTracker> pending_tx_tracker_;
  std::unique_ptr<EthBlockTracker> eth_block_tracker_;
  bool known_no_pending_tx = false;

  mojo::RemoteSet<mojom::EthTxControllerObserver> observers_;
  mojo::ReceiverSet<mojom::EthTxController> receivers_;
  mojo::Receiver<brave_wallet::mojom::KeyringControllerObserver>
      keyring_observer_receiver_{this};

  base::WeakPtrFactory<EthTxController> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_
