/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/solana_block_tracker.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"

class PrefService;

namespace brave_wallet {

class TxService;
class JsonRpcService;
class KeyringService;
class SolanaTxMeta;
class SolanaTxStateManager;
struct SolanaSignatureStatus;
struct SolanaAccountInfo;

class SolanaTxManager : public TxManager, public SolanaBlockTracker::Observer {
 public:
  SolanaTxManager(TxService* tx_service,
                  JsonRpcService* json_rpc_service,
                  KeyringService* keyring_service,
                  PrefService* prefs,
                  TxStorageDelegate* delegate,
                  AccountResolverDelegate* account_resolver_delegate);
  ~SolanaTxManager() override;

  using ProcessSolanaHardwareSignatureCallback =
      mojom::SolanaTxManagerProxy::ProcessSolanaHardwareSignatureCallback;

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

  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;

  using MakeSystemProgramTransferTxDataCallback =
      mojom::SolanaTxManagerProxy::MakeSystemProgramTransferTxDataCallback;
  using MakeTokenProgramTransferTxDataCallback =
      mojom::SolanaTxManagerProxy::MakeTokenProgramTransferTxDataCallback;
  using MakeTxDataFromBase64EncodedTransactionCallback = mojom::
      SolanaTxManagerProxy::MakeTxDataFromBase64EncodedTransactionCallback;
  using GetEstimatedTxFeeCallback =
      base::OnceCallback<void(uint64_t tx_fee,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  using GetEstimatedTxBaseFeeCallback =
      base::OnceCallback<void(std::unique_ptr<SolanaTxMeta> tx_meta,
                              const std::string& unsigned_tx,
                              uint64_t tx_fee,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  using GetSolanaTxFeeEstimationCallback =
      mojom::SolanaTxManagerProxy::GetSolanaTxFeeEstimationCallback;
  using GetSolanaTxFeeEstimationForMetaCallback =
      base::OnceCallback<void(std::unique_ptr<SolanaTxMeta> tx_meta,
                              mojom::SolanaFeeEstimationPtr fee_estimation,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void MakeSystemProgramTransferTxData(
      const std::string& from,
      const std::string& to,
      uint64_t lamports,
      MakeSystemProgramTransferTxDataCallback callback);
  void MakeTokenProgramTransferTxData(
      const std::string& chain_id,
      const std::string& spl_token_mint_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      uint64_t amount,
      uint8_t decimals,
      MakeTokenProgramTransferTxDataCallback callback);
  void MakeTxDataFromBase64EncodedTransaction(
      const std::string& encoded_transaction,
      const mojom::TransactionType tx_type,
      mojom::SolanaSendTransactionOptionsPtr send_options,
      MakeTxDataFromBase64EncodedTransactionCallback callback);
  void GetEstimatedTxBaseFee(std::unique_ptr<SolanaTxMeta> meta,
                             GetEstimatedTxBaseFeeCallback callback);
  void GetSolanaTxFeeEstimation(const std::string& chain_id,
                                const std::string& tx_meta_id,
                                GetSolanaTxFeeEstimationCallback callback);
  void GetSolanaTxFeeEstimationForMeta(
      const std::string& chain_id,
      std::unique_ptr<SolanaTxMeta> meta,
      GetSolanaTxFeeEstimationForMetaCallback callback);
  void ProcessSolanaHardwareSignature(
      const std::string& tx_meta_id,
      const std::vector<uint8_t>& signature_bytes,
      ProcessSolanaHardwareSignatureCallback callback);

  std::unique_ptr<SolanaTxMeta> GetTxForTesting(const std::string& tx_meta_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest, AddAndApproveTransaction);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest, DropTxWithInvalidBlockhash);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest,
                           DropTxWithInvalidBlockhash_DappBlockhash);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest,
                           DropTxAfterSafeDropThreshold);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest,
                           GetTransactionMessageToSign);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest,
                           ProcessSolanaHardwareSignature);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest, RetryTransaction);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest, GetEstimatedTxFee);
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest, GetSolanaTxFeeEstimation);
  friend class SolanaTxManagerUnitTest;

  mojom::CoinType GetCoinType() const override;

  // TxManager
  void UpdatePendingTransactions(
      const std::optional<std::string>& chain_id) override;

  void OnGetBlockHeightForBlockhash(std::unique_ptr<SolanaTxMeta> meta,
                                    ApproveTransactionCallback callback,
                                    const std::string& blockhash,
                                    uint64_t block_height,
                                    mojom::SolanaProviderError error,
                                    const std::string& error_message);

  void OnGetBlockHeightForBlockhashHardware(
      std::unique_ptr<SolanaTxMeta> meta,
      GetTransactionMessageToSignCallback callback,
      const std::string& blockhash,
      uint64_t block_height,
      mojom::SolanaProviderError error,
      const std::string& error_message);

  void OnGetBlockHeight(const std::string& chain_id,
                        uint64_t block_height,
                        mojom::SolanaProviderError error,
                        const std::string& error_message);

  void OnGetLatestBlockhash(std::unique_ptr<SolanaTxMeta> meta,
                            ApproveTransactionCallback callback,
                            const std::string& latest_blockhash,
                            uint64_t last_valid_block_height,
                            mojom::SolanaProviderError error,
                            const std::string& error_message);
  void OnGetLatestBlockhashHardware(
      std::unique_ptr<SolanaTxMeta> meta,
      GetTransactionMessageToSignCallback callback,
      const std::string& latest_blockhash,
      uint64_t last_valid_block_height,
      mojom::SolanaProviderError error,
      const std::string& error_message);
  void OnSendSolanaTransaction(const std::string& tx_meta_id,
                               ApproveTransactionCallback callback,
                               const std::string& tx_hash,
                               mojom::SolanaProviderError error,
                               const std::string& error_message);
  void OnGetSignatureStatuses(
      const std::string& chain_id,
      const std::vector<std::string>& tx_meta_ids,
      uint64_t block_height,
      const std::vector<std::optional<SolanaSignatureStatus>>&
          signature_statuses,
      mojom::SolanaProviderError error,
      const std::string& error_message);
  void OnGetAccountInfo(const std::string& spl_token_mint_address,
                        const std::string& from_wallet_address,
                        const std::string& to_wallet_address,
                        const std::string& from_associated_token_account,
                        const std::string& to_associated_token_account,
                        uint64_t amount,
                        uint8_t decimals,
                        mojom::SPLTokenProgram token_program,
                        MakeTokenProgramTransferTxDataCallback callback,
                        std::optional<SolanaAccountInfo> account_info,
                        mojom::SolanaProviderError error,
                        const std::string& error_message);
  void OnGetLatestBlockhashForGetEstimatedTxFee(
      std::unique_ptr<SolanaTxMeta> meta,
      GetEstimatedTxBaseFeeCallback callback,
      const std::string& latest_blockhash,
      uint64_t last_valid_block_height,
      mojom::SolanaProviderError error,
      const std::string& error_message);
  void OnGetFeeForMessage(GetEstimatedTxBaseFeeCallback callback,
                          std::unique_ptr<SolanaTxMeta> meta,
                          const std::string& unsigned_tx,
                          uint64_t tx_fee,
                          mojom::SolanaProviderError error,
                          const std::string& error_message);
  void OnGetSPLTokenProgramByMint(
      const std::string& chain_id,
      const std::string& spl_token_mint_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      uint64_t amount,
      uint8_t decimals,
      MakeTokenProgramTransferTxDataCallback callback,
      mojom::SPLTokenProgram token_program,
      mojom::SolanaProviderError error,
      const std::string& error_message);

  void OnSimulateSolanaTransaction(
      const std::string& chain_id,
      std::unique_ptr<SolanaTxMeta> meta,
      uint64_t base_fee,
      GetSolanaTxFeeEstimationForMetaCallback callback,
      uint64_t compute_units_consumed,
      mojom::SolanaProviderError error,
      const std::string& error_message);

  void OnGetRecentSolanaPrioritizationFees(
      std::unique_ptr<SolanaTxMeta> meta,
      uint64_t base_fee,
      uint64_t compute_units,
      GetSolanaTxFeeEstimationForMetaCallback callback,
      std::vector<std::pair<uint64_t, uint64_t>>& recent_fees,
      mojom::SolanaProviderError error,
      const std::string& error_message);

  void OnGetEstimatedTxBaseFee(const std::string& chain_id,
                               GetSolanaTxFeeEstimationForMetaCallback callback,
                               std::unique_ptr<SolanaTxMeta> meta,
                               const std::string& unsigned_tx,
                               uint64_t base_fee,
                               mojom::SolanaProviderError error,
                               const std::string& error_message);

  void FinishGetSolanaTxFeeEstimation(GetSolanaTxFeeEstimationCallback callback,
                                      std::unique_ptr<SolanaTxMeta> meta,
                                      mojom::SolanaFeeEstimationPtr estimation,
                                      mojom::SolanaProviderError error,
                                      const std::string& error_message);

  void ContinueAddUnapprovedTransaction(
      AddUnapprovedTransactionCallback callback,
      std::unique_ptr<SolanaTxMeta> meta,
      mojom::SolanaFeeEstimationPtr estimation,
      mojom::SolanaProviderError error,
      const std::string& error_message);

  // SolanaBlockTracker::Observer
  void OnLatestBlockhashUpdated(const std::string& chain_id,
                                const std::string& blockhash,
                                uint64_t last_valid_block_height) override;

  SolanaTxStateManager* GetSolanaTxStateManager();
  SolanaBlockTracker* GetSolanaBlockTracker();
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  base::WeakPtrFactory<SolanaTxManager> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_
