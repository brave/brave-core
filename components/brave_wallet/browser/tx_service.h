/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace value_store {
class ValueStoreFactory;
}  // namespace value_store

namespace brave_wallet {

class AccountResolverDelegate;
class JsonRpcService;
class BitcoinWalletService;
class ZCashWalletService;
class KeyringService;
class TxManager;
class TxStorageDelegate;
class TxStorageDelegateImpl;
class EthTxManager;
class SolanaTxManager;
class BitcoinTxManager;
class FilTxManager;
class ZCashTxManager;

class TxService : public mojom::TxService,
                  public mojom::EthTxManagerProxy,
                  public mojom::SolanaTxManagerProxy,
                  public mojom::FilTxManagerProxy,
                  public mojom::BtcTxManagerProxy {
 public:
  TxService(JsonRpcService* json_rpc_service,
            BitcoinWalletService* bitcoin_wallet_service,
            ZCashWalletService* zcash_wallet_service,
            KeyringService* keyring_service,
            PrefService* prefs,
            const base::FilePath& wallet_base_directory,
            scoped_refptr<base::SequencedTaskRunner> ui_task_runner);
  ~TxService() override;
  TxService(const TxService&) = delete;
  TxService operator=(const TxService&) = delete;

  template <class T>
  void Bind(mojo::PendingReceiver<T> receiver);

  // mojom::TxService
  void AddUnapprovedTransaction(
      mojom::TxDataUnionPtr tx_data_union,
      const std::string& chain_id,
      mojom::AccountIdPtr from,
      AddUnapprovedTransactionCallback callback) override;
  void AddUnapprovedTransactionWithOrigin(
      mojom::TxDataUnionPtr tx_data_union,
      const std::string& chain_id,
      mojom::AccountIdPtr from,
      const std::optional<url::Origin>& origin,
      AddUnapprovedTransactionCallback callback);
  void AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParamsPtr params,
      AddUnapprovedEvmTransactionCallback callback) override;
  void AddUnapprovedEvmTransactionWithOrigin(
      mojom::NewEvmTransactionParamsPtr params,
      const std::optional<url::Origin>& origin,
      AddUnapprovedEvmTransactionCallback callback);
  void ApproveTransaction(mojom::CoinType coin_type,
                          const std::string& chain_id,
                          const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void RejectTransaction(mojom::CoinType coin_type,
                         const std::string& chain_id,
                         const std::string& tx_meta_id,
                         RejectTransactionCallback) override;
  void GetTransactionInfo(mojom::CoinType coin_type,
                          const std::string& tx_meta_id,
                          GetTransactionInfoCallback) override;
  mojom::TransactionInfoPtr GetTransactionInfoSync(
      mojom::CoinType coin_type,
      const std::string& tx_meta_id);
  void GetAllTransactionInfo(mojom::CoinType coin_type,
                             const std::optional<std::string>& chain_id,
                             mojom::AccountIdPtr from,
                             GetAllTransactionInfoCallback) override;
  void GetPendingTransactionsCount(
      GetPendingTransactionsCountCallback callback) override;
  uint32_t GetPendingTransactionsCountSync();

  void SpeedupOrCancelTransaction(
      mojom::CoinType coin_type,
      const std::string& chain_id,
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;

  void RetryTransaction(mojom::CoinType coin_type,
                        const std::string& chain_id,
                        const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;

  void AddObserver(
      ::mojo::PendingRemote<mojom::TxServiceObserver> observer) override;

  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info);
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info);
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info);

  // Resets things back to the original state of TxService
  // To be used when the Wallet is reset / erased
  void Reset() override;

  // mojom::EthTxManagerProxy
  void MakeFilForwarderTransferData(
      const std::string& to_address,
      MakeFilForwarderTransferDataCallback callback) override;
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
  void MakeERC1155TransferFromData(
      const std::string& from,
      const std::string& to,
      const std::string& token_id,
      const std::string& value,
      const std::string& contract_address,
      MakeERC1155TransferFromDataCallback) override;

  void SetGasPriceAndLimitForUnapprovedTransaction(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      const std::string& gas_price,
      const std::string& gas_limit,
      SetGasPriceAndLimitForUnapprovedTransactionCallback callback) override;
  void SetGasFeeAndLimitForUnapprovedTransaction(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      const std::string& max_priority_fee_per_gas,
      const std::string& max_fee_per_gas,
      const std::string& gas_limit,
      SetGasFeeAndLimitForUnapprovedTransactionCallback callback) override;
  void SetDataForUnapprovedTransaction(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      const std::vector<uint8_t>& data,
      SetDataForUnapprovedTransactionCallback callback) override;
  void SetNonceForUnapprovedTransaction(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      const std::string& nonce,
      SetNonceForUnapprovedTransactionCallback) override;
  void GetNonceForHardwareTransaction(
      const std::string& tx_meta_id,
      GetNonceForHardwareTransactionCallback callback) override;
  void GetEthTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetEthTransactionMessageToSignCallback callback) override;
  void ProcessEthHardwareSignature(
      const std::string& tx_meta_id,
      mojom::EthereumSignatureVRSPtr hw_signature,
      ProcessEthHardwareSignatureCallback callback) override;
  // Gas estimation API via eth_feeHistory API
  void GetGasEstimation1559(const std::string& chain_id,
                            GetGasEstimation1559Callback callback) override;

  // mojom::SolanaTxManagerProxy
  void MakeSystemProgramTransferTxData(
      const std::string& from,
      const std::string& to,
      uint64_t lamports,
      MakeSystemProgramTransferTxDataCallback callback) override;
  void MakeTokenProgramTransferTxData(
      const std::string& chain_id,
      const std::string& spl_token_mint_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      uint64_t amount,
      uint8_t decimals,
      MakeTokenProgramTransferTxDataCallback callback) override;
  void MakeTxDataFromBase64EncodedTransaction(
      const std::string& encoded_transaction,
      const mojom::TransactionType tx_type,
      mojom::SolanaSendTransactionOptionsPtr send_options,
      MakeTxDataFromBase64EncodedTransactionCallback callback) override;
  void GetSolanaTxFeeEstimation(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      GetSolanaTxFeeEstimationCallback callback) override;
  void MakeBubbleGumProgramTransferTxData(
      const std::string& chain_id,
      const std::string& token_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      MakeBubbleGumProgramTransferTxDataCallback callback) override;
  void GetSolTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetSolTransactionMessageToSignCallback callback) override;
  void ProcessSolanaHardwareSignature(
      const std::string& tx_meta_id,
      mojom::SolanaSignaturePtr hw_signature,
      ProcessSolanaHardwareSignatureCallback callback) override;

  // mojom::FilTxManagerProxy
  void GetFilTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetFilTransactionMessageToSignCallback callback) override;
  void ProcessFilHardwareSignature(
      const std::string& tx_meta_id,
      mojom::FilecoinSignaturePtr hw_signature,
      ProcessFilHardwareSignatureCallback callback) override;

  // mojom::BtcTxManagerProxy
  void GetBtcHardwareTransactionSignData(
      const std::string& tx_meta_id,
      GetBtcHardwareTransactionSignDataCallback callback) override;
  void ProcessBtcHardwareSignature(
      const std::string& tx_meta_id,
      mojom::BitcoinSignaturePtr hw_signature,
      ProcessBtcHardwareSignatureCallback callback) override;

  TxStorageDelegate* GetDelegateForTesting();

 private:
  friend class EthereumProviderImplUnitTest;
  friend class EthTxManagerUnitTest;
  friend class SolanaTxManagerUnitTest;
  friend class FilTxManagerUnitTest;
  friend class BitcoinTxManagerUnitTest;
  friend class BraveWalletP3AUnitTest;

  void MigrateTransactionsFromPrefsToDB(PrefService* prefs);

  TxManager* GetTxManager(mojom::CoinType coin_type);
  EthTxManager* GetEthTxManager();
  SolanaTxManager* GetSolanaTxManager();
  FilTxManager* GetFilTxManager();
  BitcoinTxManager* GetBitcoinTxManager();
  ZCashTxManager* GetZCashTxManager();

  raw_ptr<PrefService> prefs_;  // NOT OWNED
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;

  scoped_refptr<value_store::ValueStoreFactory> store_factory_;
  std::unique_ptr<TxStorageDelegateImpl> delegate_;
  std::unique_ptr<AccountResolverDelegate> account_resolver_delegate_;
  base::flat_map<mojom::CoinType, std::unique_ptr<TxManager>> tx_manager_map_;

  mojo::RemoteSet<mojom::TxServiceObserver> observers_;
  mojo::ReceiverSet<mojom::TxService> tx_service_receivers_;
  mojo::ReceiverSet<mojom::EthTxManagerProxy> eth_tx_manager_receivers_;
  mojo::ReceiverSet<mojom::SolanaTxManagerProxy> solana_tx_manager_receivers_;
  mojo::ReceiverSet<mojom::FilTxManagerProxy> fil_tx_manager_receivers_;
  mojo::ReceiverSet<mojom::BtcTxManagerProxy> btc_tx_manager_receivers_;

  base::WeakPtrFactory<TxService> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_H_
