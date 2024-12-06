/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_service.h"

#include <optional>
#include <utility>

#include "base/check_is_test.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_manager.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/eth_tx_manager.h"
#include "brave/components/brave_wallet/browser/fil_tx_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/solana_tx_manager.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_manager.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "components/grit/brave_components_strings.h"
#include "components/value_store/value_store_factory_impl.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

std::string GetToAddressFromTxDataUnion(
    const mojom::TxDataUnion& tx_data_union) {
  if (tx_data_union.is_eth_tx_data_1559()) {
    return tx_data_union.get_eth_tx_data_1559()->base_data->to;
  }

  if (tx_data_union.is_eth_tx_data()) {
    return tx_data_union.get_eth_tx_data()->to;
  }

  if (tx_data_union.is_solana_tx_data()) {
    return tx_data_union.get_solana_tx_data()->to_wallet_address;
  }

  if (tx_data_union.is_fil_tx_data()) {
    return tx_data_union.get_fil_tx_data()->to;
  }

  if (tx_data_union.is_btc_tx_data()) {
    return tx_data_union.get_btc_tx_data()->to;
  }

  if (tx_data_union.is_zec_tx_data()) {
    return tx_data_union.get_zec_tx_data()->to;
  }
  NOTREACHED();
}

size_t CalculatePendingTxCount(
    const std::vector<mojom::TransactionInfoPtr>& result) {
  size_t counter = 0u;
  for (const auto& tx : result) {
    if (tx->tx_status == mojom::TransactionStatus::Unapproved) {
      counter++;
    }
  }
  return counter;
}

}  // namespace

TxService::TxService(JsonRpcService* json_rpc_service,
                     BitcoinWalletService* bitcoin_wallet_service,
                     ZCashWalletService* zcash_wallet_service,
                     KeyringService& keyring_service,
                     PrefService* prefs,
                     const base::FilePath& wallet_base_directory,
                     scoped_refptr<base::SequencedTaskRunner> ui_task_runner)
    : prefs_(prefs), json_rpc_service_(json_rpc_service), weak_factory_(this) {
  store_factory_ = base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(
      wallet_base_directory);
  delegate_ = std::make_unique<TxStorageDelegateImpl>(prefs, store_factory_,
                                                      ui_task_runner);
  account_resolver_delegate_ =
      std::make_unique<AccountResolverDelegateImpl>(keyring_service);

  tx_manager_map_[mojom::CoinType::ETH] = std::unique_ptr<TxManager>(
      new EthTxManager(*this, json_rpc_service, keyring_service, *delegate_,
                       *account_resolver_delegate_));
  tx_manager_map_[mojom::CoinType::SOL] = std::unique_ptr<TxManager>(
      new SolanaTxManager(*this, json_rpc_service, keyring_service, *delegate_,
                          *account_resolver_delegate_));
  tx_manager_map_[mojom::CoinType::FIL] = std::unique_ptr<TxManager>(
      new FilTxManager(*this, json_rpc_service, keyring_service, *delegate_,
                       *account_resolver_delegate_));
  if (IsBitcoinEnabled()) {
    if (!bitcoin_wallet_service) {
      CHECK_IS_TEST();
    } else {
      tx_manager_map_[mojom::CoinType::BTC] =
          std::make_unique<BitcoinTxManager>(*this, *bitcoin_wallet_service,
                                             keyring_service, *delegate_,
                                             *account_resolver_delegate_);
    }
  }

  if (IsZCashEnabled()) {
    if (!zcash_wallet_service) {
      CHECK_IS_TEST();
    } else {
      tx_manager_map_[mojom::CoinType::ZEC] = std::make_unique<ZCashTxManager>(
          *this, *zcash_wallet_service, keyring_service, *delegate_,
          *account_resolver_delegate_);
    }
  }
}

TxService::~TxService() = default;

TxManager* TxService::GetTxManager(mojom::CoinType coin_type) {
  auto* service = tx_manager_map_[coin_type].get();
  DCHECK(service);
  return service;
}

EthTxManager* TxService::GetEthTxManager() {
  return static_cast<EthTxManager*>(GetTxManager(mojom::CoinType::ETH));
}

SolanaTxManager* TxService::GetSolanaTxManager() {
  return static_cast<SolanaTxManager*>(GetTxManager(mojom::CoinType::SOL));
}

FilTxManager* TxService::GetFilTxManager() {
  return static_cast<FilTxManager*>(GetTxManager(mojom::CoinType::FIL));
}

BitcoinTxManager* TxService::GetBitcoinTxManager() {
  return static_cast<BitcoinTxManager*>(GetTxManager(mojom::CoinType::BTC));
}

ZCashTxManager* TxService::GetZCashTxManager() {
  return static_cast<ZCashTxManager*>(GetTxManager(mojom::CoinType::ZEC));
}

template <>
void TxService::Bind(mojo::PendingReceiver<mojom::TxService> receiver) {
  tx_service_receivers_.Add(this, std::move(receiver));
}

template <>
void TxService::Bind(mojo::PendingReceiver<mojom::EthTxManagerProxy> receiver) {
  eth_tx_manager_receivers_.Add(this, std::move(receiver));
}

template <>
void TxService::Bind(
    mojo::PendingReceiver<mojom::SolanaTxManagerProxy> receiver) {
  solana_tx_manager_receivers_.Add(this, std::move(receiver));
}

template <>
void TxService::Bind(mojo::PendingReceiver<mojom::FilTxManagerProxy> receiver) {
  fil_tx_manager_receivers_.Add(this, std::move(receiver));
}

template <>
void TxService::Bind(mojo::PendingReceiver<mojom::BtcTxManagerProxy> receiver) {
  btc_tx_manager_receivers_.Add(this, std::move(receiver));
}

void TxService::AddUnapprovedTransaction(
    mojom::TxDataUnionPtr tx_data_union,
    const std::string& chain_id,
    mojom::AccountIdPtr from,
    AddUnapprovedTransactionCallback callback) {
  CHECK_NE(from->coin, mojom::CoinType::ETH)
      << "Wallet UI must use AddUnapprovedEvmTransaction";
  AddUnapprovedTransactionWithOrigin(std::move(tx_data_union), chain_id,
                                     std::move(from), std::nullopt,
                                     std::move(callback));
}

void TxService::AddUnapprovedTransactionWithOrigin(
    mojom::TxDataUnionPtr tx_data_union,
    const std::string& chain_id,
    mojom::AccountIdPtr from,
    const std::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback) {
  if (!account_resolver_delegate_->ValidateAccountId(from)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_FROM_EMPTY));
    return;
  }

  if (BlockchainRegistry::GetInstance()->IsOfacAddress(
          GetToAddressFromTxDataUnion(*tx_data_union))) {
    std::move(callback).Run(
        false, "", l10n_util::GetStringUTF8(IDS_WALLET_OFAC_RESTRICTION));
    return;
  }

  auto coin_type = GetCoinTypeFromTxDataUnion(*tx_data_union);
  GetTxManager(coin_type)->AddUnapprovedTransaction(
      chain_id, std::move(tx_data_union), from, origin, std::move(callback));
}

void TxService::AddUnapprovedEvmTransaction(
    mojom::NewEvmTransactionParamsPtr params,
    AddUnapprovedEvmTransactionCallback callback) {
  AddUnapprovedEvmTransactionWithOrigin(std::move(params), std::nullopt,
                                        std::move(callback));
}

void TxService::AddUnapprovedEvmTransactionWithOrigin(
    mojom::NewEvmTransactionParamsPtr params,
    const std::optional<url::Origin>& origin,
    AddUnapprovedEvmTransactionCallback callback) {
  CHECK_EQ(params->from->coin, mojom::CoinType::ETH);
  if (!account_resolver_delegate_->ValidateAccountId(params->from)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_FROM_EMPTY));
    return;
  }

  if (BlockchainRegistry::GetInstance()->IsOfacAddress(params->to)) {
    std::move(callback).Run(
        false, "", l10n_util::GetStringUTF8(IDS_WALLET_OFAC_RESTRICTION));
    return;
  }

  GetEthTxManager()->AddUnapprovedEvmTransaction(std::move(params), origin,
                                                 std::move(callback));
}

void TxService::ApproveTransaction(mojom::CoinType coin_type,
                                   const std::string& chain_id,
                                   const std::string& tx_meta_id,
                                   ApproveTransactionCallback callback) {
  GetTxManager(coin_type)->ApproveTransaction(tx_meta_id, std::move(callback));
}

void TxService::RejectTransaction(mojom::CoinType coin_type,
                                  const std::string& chain_id,
                                  const std::string& tx_meta_id,
                                  RejectTransactionCallback callback) {
  GetTxManager(coin_type)->RejectTransaction(tx_meta_id, std::move(callback));
}

void TxService::GetTransactionInfo(mojom::CoinType coin_type,
                                   const std::string& tx_meta_id,
                                   GetTransactionInfoCallback callback) {
  std::move(callback).Run(GetTransactionInfoSync(coin_type, tx_meta_id));
}

mojom::TransactionInfoPtr TxService::GetTransactionInfoSync(
    mojom::CoinType coin_type,
    const std::string& tx_meta_id) {
  return GetTxManager(coin_type)->GetTransactionInfo(tx_meta_id);
}

void TxService::GetAllTransactionInfo(
    mojom::CoinType coin_type,
    const std::optional<std::string>& chain_id,
    mojom::AccountIdPtr from,
    GetAllTransactionInfoCallback callback) {
  std::optional<mojom::AccountIdPtr> from_opt =
      from ? std::move(from) : std::optional<mojom::AccountIdPtr>();
  std::move(callback).Run(GetTxManager(coin_type)->GetAllTransactionInfo(
      chain_id, std::move(from_opt)));
}

void TxService::GetPendingTransactionsCount(
    GetPendingTransactionsCountCallback callback) {
  std::move(callback).Run(GetPendingTransactionsCountSync());
}

uint32_t TxService::GetPendingTransactionsCountSync() {
  uint32_t counter = 0;

  for (auto& tx_manager : tx_manager_map_) {
    auto transactions =
        tx_manager.second->GetAllTransactionInfo(std::nullopt, std::nullopt);
    counter += CalculatePendingTxCount(transactions);
  }

  return counter;
}

void TxService::SpeedupOrCancelTransaction(
    mojom::CoinType coin_type,
    const std::string& chain_id,
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  GetTxManager(coin_type)->SpeedupOrCancelTransaction(tx_meta_id, cancel,
                                                      std::move(callback));
}

void TxService::RetryTransaction(mojom::CoinType coin_type,
                                 const std::string& chain_id,
                                 const std::string& tx_meta_id,
                                 RetryTransactionCallback callback) {
  GetTxManager(coin_type)->RetryTransaction(tx_meta_id, std::move(callback));
}

void TxService::GetEthTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetEthTransactionMessageToSignCallback callback) {
  GetEthTxManager()->GetEthTransactionMessageToSign(tx_meta_id,
                                                    std::move(callback));
}

void TxService::AddObserver(
    ::mojo::PendingRemote<mojom::TxServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void TxService::OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
  for (const auto& observer : observers_) {
    observer->OnTransactionStatusChanged(tx_info->Clone());
  }
}

void TxService::OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {
  for (const auto& observer : observers_) {
    observer->OnNewUnapprovedTx(tx_info->Clone());
  }
}

void TxService::OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) {
  for (const auto& observer : observers_) {
    observer->OnUnapprovedTxUpdated(tx_info->Clone());
  }
}

void TxService::Reset() {
  ClearTxServiceProfilePrefs(prefs_);
  delegate_->Clear();
  for (auto const& service : tx_manager_map_) {
    service.second->Reset();
  }
  for (const auto& observer : observers_) {
    observer->OnTxServiceReset();
  }
}

void TxService::MakeFilForwarderTransferData(
    const std::string& to_address,
    MakeFilForwarderTransferDataCallback callback) {
  GetEthTxManager()->MakeFilForwarderTransferData(
      FilAddress::FromAddress(to_address), std::move(callback));
}

void TxService::MakeERC20TransferData(const std::string& to_address,
                                      const std::string& amount,
                                      MakeERC20TransferDataCallback callback) {
  GetEthTxManager()->MakeERC20TransferData(to_address, amount,
                                           std::move(callback));
}

void TxService::MakeERC20ApproveData(const std::string& spender_address,
                                     const std::string& amount,
                                     MakeERC20ApproveDataCallback callback) {
  GetEthTxManager()->MakeERC20ApproveData(spender_address, amount,
                                          std::move(callback));
}

void TxService::MakeERC721TransferFromData(
    const std::string& from,
    const std::string& to,
    const std::string& token_id,
    const std::string& contract_address,
    MakeERC721TransferFromDataCallback callback) {
  GetEthTxManager()->MakeERC721TransferFromData(
      from, to, token_id, contract_address, std::move(callback));
}

void TxService::MakeERC1155TransferFromData(
    const std::string& from,
    const std::string& to,
    const std::string& token_id,
    const std::string& value,
    const std::string& contract_address,
    MakeERC1155TransferFromDataCallback callback) {
  GetEthTxManager()->MakeERC1155TransferFromData(
      from, to, token_id, value, contract_address, std::move(callback));
}

void TxService::SetGasPriceAndLimitForUnapprovedTransaction(
    const std::string& chain_id,
    const std::string& tx_meta_id,
    const std::string& gas_price,
    const std::string& gas_limit,
    SetGasPriceAndLimitForUnapprovedTransactionCallback callback) {
  GetEthTxManager()->SetGasPriceAndLimitForUnapprovedTransaction(
      tx_meta_id, gas_price, gas_limit, std::move(callback));
}

void TxService::SetGasFeeAndLimitForUnapprovedTransaction(
    const std::string& chain_id,
    const std::string& tx_meta_id,
    const std::string& max_priority_fee_per_gas,
    const std::string& max_fee_per_gas,
    const std::string& gas_limit,
    SetGasFeeAndLimitForUnapprovedTransactionCallback callback) {
  GetEthTxManager()->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, max_priority_fee_per_gas, max_fee_per_gas, gas_limit,
      std::move(callback));
}

void TxService::SetDataForUnapprovedTransaction(
    const std::string& chain_id,
    const std::string& tx_meta_id,
    const std::vector<uint8_t>& data,
    SetDataForUnapprovedTransactionCallback callback) {
  GetEthTxManager()->SetDataForUnapprovedTransaction(tx_meta_id, data,
                                                     std::move(callback));
}

void TxService::SetNonceForUnapprovedTransaction(
    const std::string& chain_id,
    const std::string& tx_meta_id,
    const std::string& nonce,
    SetNonceForUnapprovedTransactionCallback callback) {
  GetEthTxManager()->SetNonceForUnapprovedTransaction(tx_meta_id, nonce,
                                                      std::move(callback));
}

void TxService::GetNonceForHardwareTransaction(
    const std::string& tx_meta_id,
    GetNonceForHardwareTransactionCallback callback) {
  GetEthTxManager()->GetNonceForHardwareTransaction(tx_meta_id,
                                                    std::move(callback));
}

void TxService::ProcessEthHardwareSignature(
    const std::string& tx_meta_id,
    mojom::EthereumSignatureVRSPtr hw_signature,
    ProcessEthHardwareSignatureCallback callback) {
  GetEthTxManager()->ProcessEthHardwareSignature(
      tx_meta_id, std::move(hw_signature), std::move(callback));
}

void TxService::GetGasEstimation1559(const std::string& chain_id,
                                     GetGasEstimation1559Callback callback) {
  GetEthTxManager()->GetGasEstimation1559(chain_id, std::move(callback));
}

void TxService::MakeSystemProgramTransferTxData(
    const std::string& from,
    const std::string& to,
    uint64_t lamports,
    MakeSystemProgramTransferTxDataCallback callback) {
  GetSolanaTxManager()->MakeSystemProgramTransferTxData(from, to, lamports,
                                                        std::move(callback));
}

void TxService::MakeTokenProgramTransferTxData(
    const std::string& chain_id,
    const std::string& spl_token_mint_address,
    const std::string& from_wallet_address,
    const std::string& to_wallet_address,
    uint64_t amount,
    uint8_t decimals,
    MakeTokenProgramTransferTxDataCallback callback) {
  GetSolanaTxManager()->MakeTokenProgramTransferTxData(
      chain_id, spl_token_mint_address, from_wallet_address, to_wallet_address,
      amount, decimals, std::move(callback));
}

void TxService::MakeTxDataFromBase64EncodedTransaction(
    const std::string& encoded_transaction,
    const mojom::TransactionType tx_type,
    mojom::SolanaSendTransactionOptionsPtr send_options,
    MakeTxDataFromBase64EncodedTransactionCallback callback) {
  GetSolanaTxManager()->MakeTxDataFromBase64EncodedTransaction(
      encoded_transaction, std::move(tx_type), std::move(send_options),
      std::move(callback));
}

void TxService::GetSolanaTxFeeEstimation(
    const std::string& chain_id,
    const std::string& tx_meta_id,
    GetSolanaTxFeeEstimationCallback callback) {
  GetSolanaTxManager()->GetSolanaTxFeeEstimation(chain_id, tx_meta_id,
                                                 std::move(callback));
}

void TxService::MakeBubbleGumProgramTransferTxData(
    const std::string& chain_id,
    const std::string& token_address,
    const std::string& from_wallet_address,
    const std::string& to_wallet_address,
    MakeBubbleGumProgramTransferTxDataCallback callback) {
  GetSolanaTxManager()->MakeBubbleGumProgramTransferTxData(
      chain_id, token_address, from_wallet_address, to_wallet_address,
      std::move(callback));
}

void TxService::GetSolTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetSolTransactionMessageToSignCallback callback) {
  GetSolanaTxManager()->GetSolTransactionMessageToSign(tx_meta_id,
                                                       std::move(callback));
}

void TxService::ProcessSolanaHardwareSignature(
    const std::string& tx_meta_id,
    mojom::SolanaSignaturePtr hw_signature,
    ProcessSolanaHardwareSignatureCallback callback) {
  GetSolanaTxManager()->ProcessSolanaHardwareSignature(
      tx_meta_id, std::move(hw_signature), std::move(callback));
}

void TxService::GetFilTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetFilTransactionMessageToSignCallback callback) {
  GetFilTxManager()->GetFilTransactionMessageToSign(tx_meta_id,
                                                    std::move(callback));
}

void TxService::ProcessFilHardwareSignature(
    const std::string& tx_meta_id,
    mojom::FilecoinSignaturePtr hw_signature,
    ProcessFilHardwareSignatureCallback callback) {
  GetFilTxManager()->ProcessFilHardwareSignature(
      tx_meta_id, std::move(hw_signature), std::move(callback));
}

void TxService::GetBtcHardwareTransactionSignData(
    const std::string& tx_meta_id,
    GetBtcHardwareTransactionSignDataCallback callback) {
  GetBitcoinTxManager()->GetBtcHardwareTransactionSignData(tx_meta_id,
                                                           std::move(callback));
}

void TxService::ProcessBtcHardwareSignature(
    const std::string& tx_meta_id,
    mojom::BitcoinSignaturePtr hw_signature,
    ProcessBtcHardwareSignatureCallback callback) {
  GetBitcoinTxManager()->ProcessBtcHardwareSignature(
      tx_meta_id, std::move(hw_signature), std::move(callback));
}

TxStorageDelegate* TxService::GetDelegateForTesting() {
  return delegate_.get();
}

}  // namespace brave_wallet
