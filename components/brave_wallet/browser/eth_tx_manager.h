/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_block_tracker.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/fil_address.h"

namespace brave_wallet {

class EthTxManagerUnitTest;

class EthTxMeta;
class TxService;
class JsonRpcService;
class KeyringService;

class EthTxManager : public TxManager, public EthBlockTracker::Observer {
 public:
  using AddUnapprovedEvmTransactionCallback =
      mojom::TxService::AddUnapprovedEvmTransactionCallback;

  EthTxManager(TxService& tx_service,
               JsonRpcService* json_rpc_service,
               KeyringService& keyring_service,
               TxStorageDelegate& delegate,
               AccountResolverDelegate& account_resolver_delegate);
  ~EthTxManager() override;
  EthTxManager(const EthTxManager&) = delete;
  EthTxManager operator=(const EthTxManager&) = delete;

  void AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParamsPtr params,
      const std::optional<url::Origin>& origin,
      AddUnapprovedEvmTransactionCallback callback);

  // TxManager
  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::TxDataUnionPtr tx_data_union,
                                const mojom::AccountIdPtr& from,
                                const std::optional<url::Origin>& origin,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;
  // Resets things back to the original state of EthTxManager
  // To be used when the Wallet is reset / erased
  void Reset() override;

  using MakeERC20TransferDataCallback =
      mojom::EthTxManagerProxy::MakeERC20TransferDataCallback;
  using MakeERC20ApproveDataCallback =
      mojom::EthTxManagerProxy::MakeERC20ApproveDataCallback;
  using MakeERC721TransferFromDataCallback =
      mojom::EthTxManagerProxy::MakeERC721TransferFromDataCallback;
  using MakeERC1155TransferFromDataCallback =
      mojom::EthTxManagerProxy::MakeERC1155TransferFromDataCallback;
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
  using GetEthTransactionMessageToSignCallback =
      mojom::EthTxManagerProxy::GetEthTransactionMessageToSignCallback;
  using ProcessEthHardwareSignatureCallback =
      mojom::EthTxManagerProxy::ProcessEthHardwareSignatureCallback;
  using GetGasEstimation1559Callback =
      mojom::EthTxManagerProxy::GetGasEstimation1559Callback;
  using MakeFilForwarderDataCallback =
      mojom::EthTxManagerProxy::MakeFilForwarderTransferDataCallback;

  void MakeFilForwarderTransferData(const FilAddress& fil_address,
                                    MakeFilForwarderDataCallback callback);
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

  void MakeERC1155TransferFromData(const std::string& from,
                                   const std::string& to,
                                   const std::string& token_id,
                                   const std::string& value,
                                   const std::string& contract_address,
                                   MakeERC1155TransferFromDataCallback);

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
  void GetEthTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetEthTransactionMessageToSignCallback callback);
  void ProcessEthHardwareSignature(
      const std::string& tx_meta_id,
      mojom::EthereumSignatureVRSPtr hw_signature,
      ProcessEthHardwareSignatureCallback callback);

  // Gas estimation API via eth_feeHistory API
  void GetGasEstimation1559(const std::string& chain_id,
                            GetGasEstimation1559Callback callback);

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

  mojom::CoinType GetCoinType() const override;

  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::TxDataPtr tx_data,
                                const mojom::AccountIdPtr& from,
                                const url::Origin& origin,
                                AddUnapprovedTransactionCallback);
  void AddUnapproved1559Transaction(const std::string& chain_id,
                                    mojom::TxData1559Ptr tx_data,
                                    const mojom::AccountIdPtr& from,
                                    const url::Origin& origin,
                                    AddUnapprovedTransactionCallback);

  void NotifyUnapprovedTxUpdated(TxMeta* meta);
  void OnConnectionError();
  void OnGetNextNonce(std::unique_ptr<EthTxMeta> meta,
                      ApproveTransactionCallback callback,
                      bool success,
                      uint256_t nonce);
  void OnGetNextNonceForHardware(
      std::unique_ptr<EthTxMeta> meta,
      GetNonceForHardwareTransactionCallback callback,
      bool success,
      uint256_t nonce);
  void PublishTransaction(const std::string& chain_id,
                          const std::string& tx_meta_id,
                          const std::string& signed_transaction,
                          ApproveTransactionCallback callback);
  void OnPublishTransaction(const std::string& chain_id,
                            const std::string& tx_meta_id,
                            ApproveTransactionCallback callback,
                            const std::string& tx_hash,
                            mojom::ProviderError error,
                            const std::string& error_message);
  void OnGetGasPrice(const std::string& chain_id,
                     const mojom::AccountIdPtr& from,
                     const url::Origin& origin,
                     const std::string& to,
                     const std::string& value,
                     const std::string& data,
                     const std::string& gas_limit,
                     std::unique_ptr<EthTransaction> tx,
                     AddUnapprovedTransactionCallback callback,
                     bool sign_only,
                     const std::string& result,
                     mojom::ProviderError error,
                     const std::string& error_message);
  void ContinueAddUnapprovedTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
      std::unique_ptr<EthTransaction> tx,
      AddUnapprovedTransactionCallback callback,
      bool sign_only,
      const std::string& result,
      mojom::ProviderError error,
      const std::string& error_message);
  void OnGetGasEstimation1559(
      GetGasEstimation1559Callback callback,
      const std::string& chain_id,
      const std::vector<std::string>& base_fee_per_gas,
      const std::vector<double>& gas_used_ratio,
      const std::string& oldest_block,
      const std::vector<std::vector<std::string>>& reward,
      mojom::ProviderError error,
      const std::string& error_message);
  void OnGetGasOracleForUnapprovedTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from,
      const url::Origin& origin,
      const std::string& to,
      const std::string& value,
      const std::string& data,
      const std::string& gas_limit,
      std::unique_ptr<Eip1559Transaction> tx,
      AddUnapprovedTransactionCallback callback,
      bool sign_only,
      mojom::GasEstimation1559Ptr gas_estimation);
  void UpdatePendingTransactions(
      const std::optional<std::string>& chain_id) override;

  void ContinueSpeedupOrCancelTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
      const std::string& gas_limit,
      std::unique_ptr<EthTransaction> tx,
      SpeedupOrCancelTransactionCallback callback,
      const std::string& result,
      mojom::ProviderError error,
      const std::string& error_message);
  void ContinueSpeedupOrCancel1559Transaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
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

  void ContinueProcessHardwareSignature(
      ProcessEthHardwareSignatureCallback callback,
      bool status,
      mojom::ProviderErrorUnionPtr error_union,
      const std::string& error_message);

  void OnGetBaseFeePerGas(GetGasEstimation1559Callback callback,
                          const std::string& base_fee_per_gas,
                          mojom::ProviderError error,
                          const std::string& error_message);

  // EthBlockTracker::Observer:
  void OnLatestBlock(const std::string& chain_id,
                     uint256_t block_num) override {}
  void OnNewBlock(const std::string& chain_id, uint256_t block_num) override;

  EthTxStateManager& GetEthTxStateManager();
  EthBlockTracker& GetEthBlockTracker();

  std::unique_ptr<EthNonceTracker> nonce_tracker_;
  std::unique_ptr<EthPendingTxTracker> pending_tx_tracker_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;

  base::WeakPtrFactory<EthTxManager> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_MANAGER_H_
