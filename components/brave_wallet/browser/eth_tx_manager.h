/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_block_tracker.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

class PrefService;

namespace brave_wallet {

class EthTxManagerUnitTest;

class EthTxMeta;
class TxService;
class JsonRpcService;
class KeyringService;
class EthNonceTracker;

class EthTxManager : public TxManager, public EthBlockTracker::Observer {
 public:
  EthTxManager(TxService* tx_service,
               JsonRpcService* json_rpc_service,
               KeyringService* keyring_service,
               PrefService* prefs);
  ~EthTxManager() override;
  EthTxManager(const EthTxManager&) = delete;
  EthTxManager operator=(const EthTxManager&) = delete;

  // TxManager
  void AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                const std::string& from,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void GetAllTransactionInfo(const std::string& from,
                             GetAllTransactionInfoCallback) override;

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;
  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;

  // Resets things back to the original state of EthTxManager
  // To be used when the Wallet is reset / erased
  void Reset() override;

  using MakeERC20TransferDataCallback =
      mojom::EthTxManagerProxy::MakeERC20TransferDataCallback;
  using MakeERC20ApproveDataCallback =
      mojom::EthTxManagerProxy::MakeERC20ApproveDataCallback;
  using MakeERC721TransferFromDataCallback =
      mojom::EthTxManagerProxy::MakeERC721TransferFromDataCallback;
  using SetGasPriceAndLimitForUnapprovedTransactionCallback = mojom::
      EthTxManagerProxy::SetGasPriceAndLimitForUnapprovedTransactionCallback;
  using SetGasFeeAndLimitForUnapprovedTransactionCallback = mojom::
      EthTxManagerProxy::SetGasFeeAndLimitForUnapprovedTransactionCallback;
  using SetDataForUnapprovedTransactionCallback =
      mojom::EthTxManagerProxy::SetDataForUnapprovedTransactionCallback;
  using SetNonceForUnapprovedTransactionCallback =
      mojom::EthTxManagerProxy::SetNonceForUnapprovedTransactionCallback;
  using GetNonceForHardwareTransactionCallback =
      mojom::EthTxManagerProxy::GetNonceForHardwareTransactionCallback;
  using ProcessHardwareSignatureCallback =
      mojom::EthTxManagerProxy::ProcessHardwareSignatureCallback;
  using GetGasEstimation1559Callback =
      mojom::EthTxManagerProxy::GetGasEstimation1559Callback;

  void MakeERC20TransferData(const std::string& to_address,
                             const std::string& amount,
                             MakeERC20TransferDataCallback);
  void MakeERC20ApproveData(const std::string& to_address,
                            const std::string& amount,
                            MakeERC20ApproveDataCallback);

  void MakeERC721TransferFromData(const std::string& from,
                                  const std::string& to,
                                  const std::string& token_id,
                                  const std::string& contract_address,
                                  MakeERC721TransferFromDataCallback);

  void SetGasPriceAndLimitForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::string& gas_price,
      const std::string& gas_limit,
      SetGasPriceAndLimitForUnapprovedTransactionCallback callback);
  void SetGasFeeAndLimitForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::string& max_priority_fee_per_gas,
      const std::string& max_fee_per_gas,
      const std::string& gas_limit,
      SetGasFeeAndLimitForUnapprovedTransactionCallback callback);
  void SetDataForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::vector<uint8_t>& data,
      SetDataForUnapprovedTransactionCallback callback);
  void SetNonceForUnapprovedTransaction(
      const std::string& tx_meta_id,
      const std::string& nonce,
      SetNonceForUnapprovedTransactionCallback);
  void GetNonceForHardwareTransaction(
      const std::string& tx_meta_id,
      GetNonceForHardwareTransactionCallback callback);
  void ProcessHardwareSignature(const std::string& tx_meta_id,
                                const std::string& v,
                                const std::string& r,
                                const std::string& s,
                                ProcessHardwareSignatureCallback callback);

  // Gas estimation API via eth_feeHistory API
  void GetGasEstimation1559(GetGasEstimation1559Callback callback);

  static bool ValidateTxData(const mojom::TxDataPtr& tx_data,
                             std::string* error);
  static bool ValidateTxData1559(const mojom::TxData1559Ptr& tx_data,
                                 std::string* error);
  std::unique_ptr<EthTxMeta> GetTxForTesting(const std::string& tx_meta_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthTxManagerUnitTest, TestSubmittedToConfirmed);
  FRIEND_TEST_ALL_PREFIXES(EthTxManagerUnitTest, RetryTransaction);
  FRIEND_TEST_ALL_PREFIXES(EthTxManagerUnitTest, Reset);
  friend class EthTxManagerUnitTest;

  void AddUnapprovedTransaction(mojom::TxDataPtr tx_data,
                                const std::string& from,
                                AddUnapprovedTransactionCallback);
  void AddUnapproved1559Transaction(mojom::TxData1559Ptr tx_data,
                                    const std::string& from,
                                    AddUnapprovedTransactionCallback);

  void NotifyUnapprovedTxUpdated(TxMeta* meta);
  void OnConnectionError();
  void OnGetNextNonce(std::unique_ptr<EthTxMeta> meta,
                      uint256_t chain_id,
                      ApproveTransactionCallback callback,
                      bool success,
                      uint256_t nonce);
  void OnGetNextNonceForHardware(
      std::unique_ptr<EthTxMeta> meta,
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
  void OnGetGasEstimation1559(
      GetGasEstimation1559Callback callback,
      const std::vector<std::string>& base_fee_per_gas,
      const std::vector<double>& gas_used_ratio,
      const std::string& oldest_block,
      const std::vector<std::vector<std::string>>& reward,
      mojom::ProviderError error,
      const std::string& error_message);
  void OnGetGasOracleForUnapprovedTransaction(
      const std::string& from,
      const std::string& to,
      const std::string& value,
      const std::string& data,
      const std::string& gas_limit,
      std::unique_ptr<Eip1559Transaction> tx,
      AddUnapprovedTransactionCallback callback,
      mojom::GasEstimation1559Ptr gas_estimation);
  void UpdatePendingTransactions() override;

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

  // EthBlockTracker::Observer:
  void OnLatestBlock(uint256_t block_num) override {}
  void OnNewBlock(uint256_t block_num) override;

  EthTxStateManager* GetEthTxStateManager();
  EthBlockTracker* GetEthBlockTracker();

  std::unique_ptr<EthNonceTracker> nonce_tracker_;
  std::unique_ptr<EthPendingTxTracker> pending_tx_tracker_;

  base::WeakPtrFactory<EthTxManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_MANAGER_H_
