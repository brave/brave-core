/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace brave_wallet {

class JsonRpcService;
class KeyringService;
class TxManager;
class EthTxManager;
class SolanaTxManager;
class FilTxManager;

class TxService : public KeyedService,
                  public mojom::TxService,
                  public mojom::EthTxManagerProxy,
                  public mojom::SolanaTxManagerProxy,
                  public mojom::FilTxManagerProxy {
 public:
  TxService(JsonRpcService* json_rpc_service,
            KeyringService* keyring_service,
            PrefService* prefs);
  ~TxService() override;
  TxService(const TxService&) = delete;
  TxService operator=(const TxService&) = delete;

  mojo::PendingRemote<mojom::TxService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::TxService> receiver);
  mojo::PendingRemote<mojom::EthTxManagerProxy> MakeEthTxManagerProxyRemote();
  void BindEthTxManagerProxy(
      mojo::PendingReceiver<mojom::EthTxManagerProxy> receiver);
  mojo::PendingRemote<mojom::SolanaTxManagerProxy>
  MakeSolanaTxManagerProxyRemote();
  void BindSolanaTxManagerProxy(
      mojo::PendingReceiver<mojom::SolanaTxManagerProxy> receiver);

  mojo::PendingRemote<mojom::FilTxManagerProxy> MakeFilTxManagerProxyRemote();
  void BindFilTxManagerProxy(
      mojo::PendingReceiver<mojom::FilTxManagerProxy> receiver);

  // mojom::TxService
  void AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                const std::string& from,
                                const absl::optional<url::Origin>& origin,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(mojom::CoinType coin_type,
                          const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void RejectTransaction(mojom::CoinType coin_type,
                         const std::string& tx_meta_id,
                         RejectTransactionCallback) override;
  void GetTransactionInfo(mojom::CoinType coin_type,
                          const std::string& tx_meta_id,
                          GetTransactionInfoCallback) override;
  void GetAllTransactionInfo(mojom::CoinType coin_type,
                             const std::string& from,
                             GetAllTransactionInfoCallback) override;

  void SpeedupOrCancelTransaction(
      mojom::CoinType coin_type,
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;

  void RetryTransaction(mojom::CoinType coin_type,
                        const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;

  void GetTransactionMessageToSign(
      mojom::CoinType coin_type,
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;

  void AddObserver(
      ::mojo::PendingRemote<mojom::TxServiceObserver> observer) override;

  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info);
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info);
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info);

  // Resets things back to the original state of TxService
  // To be used when the Wallet is reset / erased
  void Reset() override;

  // mojom::EthTxManagerProxy
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
  void ProcessHardwareSignature(
      const std::string& tx_meta_id,
      const std::string& v,
      const std::string& r,
      const std::string& s,
      ProcessHardwareSignatureCallback callback) override;
  // Gas estimation API via eth_feeHistory API
  void GetGasEstimation1559(GetGasEstimation1559Callback callback) override;

  // mojom::SolanaTxManagerProxy
  void MakeSystemProgramTransferTxData(
      const std::string& from,
      const std::string& to,
      uint64_t lamports,
      MakeSystemProgramTransferTxDataCallback callback) override;
  void MakeTokenProgramTransferTxData(
      const std::string& spl_token_mint_address,
      const std::string& from_wallet_address,
      const std::string& to_wallet_address,
      uint64_t amount,
      MakeTokenProgramTransferTxDataCallback callback) override;
  void GetEstimatedTxFee(const std::string& tx_meta_id,
                         GetEstimatedTxFeeCallback callback) override;

  // mojom::FilTxManagerProxy
  void ProcessFilHardwareSignature(
      const std::string& tx_meta_id,
      const std::string& signed_message,
      ProcessFilHardwareSignatureCallback callback) override;

 private:
  friend class EthTxManagerUnitTest;
  friend class SolanaTxManagerUnitTest;
  friend class FilTxManagerUnitTest;

  TxManager* GetTxManager(mojom::CoinType coin_type);
  EthTxManager* GetEthTxManager();
  SolanaTxManager* GetSolanaTxManager();
  FilTxManager* GetFilTxManager();

  raw_ptr<PrefService> prefs_;  // NOT OWNED
  base::flat_map<mojom::CoinType, std::unique_ptr<TxManager>> tx_manager_map_;
  mojo::RemoteSet<mojom::TxServiceObserver> observers_;
  mojo::ReceiverSet<mojom::TxService> tx_service_receivers_;
  mojo::ReceiverSet<mojom::EthTxManagerProxy> eth_tx_manager_receivers_;
  mojo::ReceiverSet<mojom::SolanaTxManagerProxy> solana_tx_manager_receivers_;
  mojo::ReceiverSet<mojom::FilTxManagerProxy> fil_tx_manager_receivers_;

  base::WeakPtrFactory<TxService> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_H_
