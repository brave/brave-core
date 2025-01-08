/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_provider_impl.h"

#include <optional>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// When onlyIfTrusted is true, the request would be rejected when selected
// account doesn't have permission.
constexpr char kOnlyIfTrustedOption[] = "onlyIfTrusted";
constexpr char kMessage[] = "message";
constexpr char kPublicKey[] = "publicKey";
constexpr char kSignature[] = "signature";
constexpr char kOptions[] = "options";

}  // namespace

SolanaProviderImpl::SolanaProviderImpl(
    HostContentSettingsMap& host_content_settings_map,
    BraveWalletService* brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate)
    : host_content_settings_map_(host_content_settings_map),
      brave_wallet_service_(brave_wallet_service),
      keyring_service_(brave_wallet_service->keyring_service()),
      tx_service_(brave_wallet_service->tx_service()),
      json_rpc_service_(brave_wallet_service->json_rpc_service()),
      delegate_(std::move(delegate)),
      weak_factory_(this) {
  DCHECK(keyring_service_);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(tx_service_);
  tx_service_->AddObserver(tx_observer_receiver_.BindNewPipeAndPassRemote());
  host_content_settings_map_observation_.Observe(&*host_content_settings_map_);
}

SolanaProviderImpl::~SolanaProviderImpl() = default;

void SolanaProviderImpl::Init(
    ::mojo::PendingRemote<mojom::SolanaEventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
  }
}

void SolanaProviderImpl::Connect(std::optional<base::Value::Dict> arg,
                                 ConnectCallback callback) {
  DCHECK(delegate_);
  if (delegate_->IsPermissionDenied(mojom::CoinType::SOL)) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
    return;
  }
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    // Prompt users to create a Solana account. If wallet is not setup, users
    // will be lead to onboarding first.
    if (!account_creation_shown_) {
      delegate_->ShowAccountCreation(mojom::CoinType::SOL);
      account_creation_shown_ = true;
    }
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            "");
    return;
  }

  bool is_eagerly_connect = false;
  if (arg.has_value()) {
    is_eagerly_connect = arg->FindBool(kOnlyIfTrustedOption).value_or(false);
  }

  if (keyring_service_->IsLockedSync()) {
    // Reject the request when we are already waiting for unlock and we will
    // also reject eagerly connect when wallet is locked.
    if (pending_connect_callback_ || is_eagerly_connect) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kUserRejectedRequest,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
      return;
    }
    pending_connect_callback_ = std::move(callback);
    pending_connect_arg_ = std::move(arg);
    keyring_service_->RequestUnlock();
    delegate_->ShowPanel();
    return;
  }

  const bool allowed =
      delegate_->IsAccountAllowed(mojom::CoinType::SOL, account->address);

  if (allowed) {
    std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                            account->address);
    delegate_->AddSolanaConnectedAccount(account->address);
  } else if (!allowed && is_eagerly_connect) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
  } else {
    std::vector<mojom::AccountInfoPtr> sol_accounts;
    std::vector<std::string> addresses;
    for (const auto& account_info : keyring_service_->GetAllAccountInfos()) {
      if (account_info->account_id->coin == mojom::CoinType::SOL) {
        sol_accounts.push_back(account_info.Clone());
        addresses.push_back(account_info->address);
      }
    }
    // filter out already permitted accounts if exists
    const auto allowed_accounts =
        delegate_->GetAllowedAccounts(mojom::CoinType::SOL, addresses);
    if (allowed_accounts) {
      std::erase_if(addresses, [&allowed_accounts](const auto& address) {
        return base::Contains(*allowed_accounts, address);
      });
    }
    delegate_->RequestPermissions(
        mojom::CoinType::SOL, addresses,
        base::BindOnce(&SolanaProviderImpl::OnConnect,
                       weak_factory_.GetWeakPtr(), std::move(sol_accounts),
                       std::move(callback)));
  }

  // To show wallet icon on android if wallet is unlocked
  delegate_->WalletInteractionDetected();
}

void SolanaProviderImpl::Disconnect() {
  DCHECK(keyring_service_);
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (account && IsAccountConnected(*account)) {
    delegate_->RemoveSolanaConnectedAccount(account->address);
    events_listener_->DisconnectEvent();
  }
}

void SolanaProviderImpl::IsConnected(IsConnectedCallback callback) {
  DCHECK(keyring_service_);
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    std::move(callback).Run(false);
  } else {
    std::move(callback).Run(IsAccountConnected(*account));
  }
}

void SolanaProviderImpl::GetPublicKey(GetPublicKeyCallback callback) {
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    std::move(callback).Run("");
    return;
  }
  if (IsAccountConnected(*account)) {
    std::move(callback).Run(account->address);
  } else {
    std::move(callback).Run("");
  }
}

std::optional<std::pair<SolanaMessage, std::vector<uint8_t>>>
SolanaProviderImpl::GetDeserializedMessage(
    const std::string& encoded_serialized_msg) {
  std::vector<uint8_t> message_bytes;
  if (!Base58Decode(encoded_serialized_msg, &message_bytes, kSolanaMaxTxSize,
                    false)) {
    return std::nullopt;
  }

  auto msg = SolanaMessage::Deserialize(message_bytes);
  if (!msg) {
    return std::nullopt;
  }

  // Note: We cannot check Base58Encode(msg->Serialize()) is equal to the
  // original encoded message passed in because the order of accounts with the
  // same is_signer and is_writable value can be different between different
  // implementation. See https://github.com/brave/brave-browser/issues/23542
  // for more details.

  return std::make_pair(std::move(*msg), std::move(message_bytes));
}

void SolanaProviderImpl::SignTransaction(
    mojom::SolanaSignTransactionParamPtr param,
    SignTransactionCallback callback) {
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>(),
                            mojom::SolanaMessageVersion::kLegacy);
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            std::vector<uint8_t>(),
                            mojom::SolanaMessageVersion::kLegacy);
    return;
  }
  auto msg_pair = GetDeserializedMessage(param->encoded_serialized_msg);
  if (!msg_pair) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>(),
                            mojom::SolanaMessageVersion::kLegacy);
    return;
  }

  const std::string chain_id = json_rpc_service_->GetChainIdSync(
      mojom::CoinType::SOL, delegate_->GetOrigin());
  const std::string blockhash = msg_pair->first.recent_blockhash();
  auto internal_callback = base::BindOnce(
      &SolanaProviderImpl::ContinueSignTransaction, weak_factory_.GetWeakPtr(),
      std::move(msg_pair), std::move(param), account.Clone(), chain_id,
      std::move(callback));
  json_rpc_service_->IsSolanaBlockhashValid(chain_id, blockhash, std::nullopt,
                                            std::move(internal_callback));
}

void SolanaProviderImpl::ContinueSignTransaction(
    std::optional<std::pair<SolanaMessage, std::vector<uint8_t>>> msg_pair,
    mojom::SolanaSignTransactionParamPtr param,
    const mojom::AccountInfoPtr& account,
    const std::string& chain_id,
    SignTransactionCallback callback,
    bool is_valid,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess || !is_valid) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_BLOCKHASH_ERROR),
        std::vector<uint8_t>(), mojom::SolanaMessageVersion::kLegacy);
    return;
  }

  auto tx = std::make_unique<SolanaTransaction>(std::move(msg_pair->first),
                                                std::move(param));
  tx->set_tx_type(mojom::TransactionType::SolanaDappSignTransaction);
  std::vector<mojom::SolanaTxDataPtr> tx_datas;
  tx_datas.push_back(tx->ToSolanaTxData());
  std::vector<std::vector<uint8_t>> raw_messages;
  raw_messages.push_back(std::move(msg_pair->second));

  auto request = mojom::SignSolTransactionsRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, account->account_id.Clone(),
      account->address, std::move(tx_datas), std::move(raw_messages), chain_id);
  brave_wallet_service_->AddSignSolTransactionsRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignTransactionRequestProcessed,
                     weak_factory_.GetWeakPtr(), std::move(tx), account.Clone(),
                     std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::OnSignTransactionRequestProcessed(
    std::unique_ptr<SolanaTransaction> tx,
    const mojom::AccountInfoPtr& account,
    SignTransactionCallback callback,
    bool approved,
    std::vector<mojom::SolanaSignaturePtr> hw_signatures,
    const std::optional<std::string>& error) {
  if (error && !error->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError, *error,
                            std::vector<uint8_t>(),
                            mojom::SolanaMessageVersion::kLegacy);
    return;
  }

  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::vector<uint8_t>(), mojom::SolanaMessageVersion::kLegacy);
    return;
  }

  std::optional<std::vector<uint8_t>> signed_tx;
  if (account->account_id->kind != mojom::AccountKind::kHardware) {
    signed_tx = tx->GetSignedTransactionBytes(keyring_service_,
                                              account->account_id, nullptr);
  } else if (hw_signatures.size() == 1 && hw_signatures[0]) {  // hardware
    signed_tx = tx->GetSignedTransactionBytes(
        keyring_service_, account->account_id, hw_signatures[0]);
  }

  if (!signed_tx || signed_tx->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>(),
                            mojom::SolanaMessageVersion::kLegacy);
    return;
  }

  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "", *signed_tx,
                          tx->message()->version());
}

void SolanaProviderImpl::SignAllTransactions(
    std::vector<mojom::SolanaSignTransactionParamPtr> params,
    SignAllTransactionsCallback callback) {
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<std::vector<uint8_t>>(),
                            std::vector<mojom::SolanaMessageVersion>());
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            std::vector<std::vector<uint8_t>>(),
                            std::vector<mojom::SolanaMessageVersion>());
    return;
  }

  std::vector<mojom::SolanaTxDataPtr> tx_datas;
  std::vector<std::unique_ptr<SolanaTransaction>> txs;
  std::vector<std::vector<uint8_t>> raw_messages;
  std::vector<std::string> blockhashs;
  const std::string chain_id = json_rpc_service_->GetChainIdSync(
      mojom::CoinType::SOL, delegate_->GetOrigin());
  for (auto& param : params) {
    auto msg_pair = GetDeserializedMessage(param->encoded_serialized_msg);
    if (!msg_pair) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
          std::vector<std::vector<uint8_t>>(),
          std::vector<mojom::SolanaMessageVersion>());
      return;
    }

    blockhashs.emplace_back(msg_pair->first.recent_blockhash());
    auto tx = std::make_unique<SolanaTransaction>(std::move(msg_pair->first),
                                                  std::move(param));
    tx->set_tx_type(mojom::TransactionType::SolanaDappSignTransaction);
    raw_messages.push_back(std::move(msg_pair->second));
    tx_datas.push_back(tx->ToSolanaTxData());
    txs.push_back(std::move(tx));
  }

  const auto barrier_callback = base::BarrierCallback<bool>(
      params.size(),
      base::BindOnce(&SolanaProviderImpl::ContinueSignAllTransactions,
                     weak_factory_.GetWeakPtr(), std::move(tx_datas),
                     std::move(txs), std::move(raw_messages),
                     std::move(account),
                     json_rpc_service_->GetChainIdSync(mojom::CoinType::SOL,
                                                       delegate_->GetOrigin()),
                     std::move(callback)));
  for (const auto& blockhash : blockhashs) {
    json_rpc_service_->IsSolanaBlockhashValid(
        chain_id, blockhash, std::nullopt,
        base::BindOnce(
            [](base::OnceCallback<void(bool)> barrier_callback, bool is_valid,
               mojom::SolanaProviderError error,
               const std::string& error_message) {
              std::move(barrier_callback)
                  .Run(is_valid &&
                       error == mojom::SolanaProviderError::kSuccess);
            },
            barrier_callback));
  }
}

void SolanaProviderImpl::ContinueSignAllTransactions(
    std::vector<mojom::SolanaTxDataPtr> tx_datas,
    std::vector<std::unique_ptr<SolanaTransaction>> txs,
    std::vector<std::vector<uint8_t>> raw_messages,
    mojom::AccountInfoPtr account,
    const std::string& chain_id,
    SignAllTransactionsCallback callback,
    const std::vector<bool>& is_valids) {
  if (base::ranges::any_of(is_valids,
                           [](auto is_valid) { return !is_valid; })) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_BLOCKHASH_ERROR),
        std::vector<std::vector<uint8_t>>(),
        std::vector<mojom::SolanaMessageVersion>());
    return;
  }

  auto request = mojom::SignSolTransactionsRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, account->account_id.Clone(),
      account->address, std::move(tx_datas), std::move(raw_messages), chain_id);

  brave_wallet_service_->AddSignSolTransactionsRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignAllTransactionsRequestProcessed,
                     weak_factory_.GetWeakPtr(), std::move(txs),
                     std::move(account), std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::OnSignAllTransactionsRequestProcessed(
    const std::vector<std::unique_ptr<SolanaTransaction>>& txs,
    mojom::AccountInfoPtr account,
    SignAllTransactionsCallback callback,
    bool approved,
    std::vector<mojom::SolanaSignaturePtr> hw_signatures,
    const std::optional<std::string>& error) {
  if (error && !error->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError, *error,
                            std::vector<std::vector<uint8_t>>(),
                            std::vector<mojom::SolanaMessageVersion>());
    return;
  }

  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::vector<std::vector<uint8_t>>(),
        std::vector<mojom::SolanaMessageVersion>());
    return;
  }

  bool is_hardware_account =
      account->account_id->kind == mojom::AccountKind::kHardware;
  if (is_hardware_account &&
      (hw_signatures.size() != txs.size() ||
       base::ranges::any_of(hw_signatures, [](auto& sig) { return !sig; }))) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<std::vector<uint8_t>>(),
                            std::vector<mojom::SolanaMessageVersion>());
    return;
  }

  std::vector<std::vector<uint8_t>> signed_txs;
  std::vector<mojom::SolanaMessageVersion> versions;
  for (size_t i = 0; i < txs.size(); ++i) {
    std::optional<std::vector<uint8_t>> signed_tx;
    if (!is_hardware_account) {
      signed_tx = txs[i]->GetSignedTransactionBytes(
          keyring_service_, account->account_id, nullptr);
    } else {
      CHECK(hw_signatures[i]);
      signed_tx = txs[i]->GetSignedTransactionBytes(
          keyring_service_, account->account_id, hw_signatures[i]);
    }

    if (!signed_tx || signed_tx->empty()) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
          std::vector<std::vector<uint8_t>>(),
          std::vector<mojom::SolanaMessageVersion>());
      return;
    }

    versions.push_back(txs[i]->message()->version());
    signed_txs.push_back(std::move(*signed_tx));
  }

  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "", signed_txs,
                          versions);
}

void SolanaProviderImpl::SignAndSendTransaction(
    mojom::SolanaSignTransactionParamPtr param,
    std::optional<base::Value::Dict> send_options,
    SignAndSendTransactionCallback callback) {
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            base::Value::Dict());
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            base::Value::Dict());
    return;
  }

  auto msg_pair = GetDeserializedMessage(param->encoded_serialized_msg);
  if (!msg_pair) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            base::Value::Dict());
    return;
  }

  SolanaTransaction tx =
      SolanaTransaction(std::move(msg_pair->first), std::move(param));
  tx.set_tx_type(mojom::TransactionType::SolanaDappSignAndSendTransaction);
  tx.set_send_options(
      SolanaTransaction::SendOptions::FromValue(std::move(send_options)));

  tx_service_->AddUnapprovedTransactionWithOrigin(
      mojom::TxDataUnion::NewSolanaTxData(tx.ToSolanaTxData()),
      json_rpc_service_->GetChainIdSync(mojom::CoinType::SOL,
                                        delegate_->GetOrigin()),
      account->account_id.Clone(), delegate_->GetOrigin(),
      base::BindOnce(&SolanaProviderImpl::OnAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void SolanaProviderImpl::OnAddUnapprovedTransaction(
    SignAndSendTransactionCallback callback,
    bool success,
    const std::string& tx_meta_id,
    const std::string& error_message) {
  if (!success) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            base::Value::Dict());
    return;
  }

  sign_and_send_tx_callbacks_[tx_meta_id] = std::move(callback);
  delegate_->ShowPanel();
}

void SolanaProviderImpl::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  auto tx_status = tx_info->tx_status;
  if (tx_status != mojom::TransactionStatus::Submitted &&
      tx_status != mojom::TransactionStatus::Rejected &&
      tx_status != mojom::TransactionStatus::Error) {
    return;
  }

  std::string tx_meta_id = tx_info->id;
  if (!sign_and_send_tx_callbacks_.contains(tx_meta_id)) {
    return;
  }

  auto callback = std::move(sign_and_send_tx_callbacks_[tx_meta_id]);
  base::Value::Dict result;
  if (tx_status == mojom::TransactionStatus::Submitted) {
    CHECK(tx_info->from_address);
    result.Set(kPublicKey, *tx_info->from_address);
    result.Set(kSignature, tx_info->tx_hash);
    std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                            std::move(result));
  } else if (tx_status == mojom::TransactionStatus::Rejected) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::move(result));
  } else if (tx_status == mojom::TransactionStatus::Error) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_ERROR),
        std::move(result));
  } else {
    NOTREACHED_IN_MIGRATION();
  }
  sign_and_send_tx_callbacks_.erase(tx_meta_id);
}

void SolanaProviderImpl::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  if (content_type != ContentSettingsType::BRAVE_SOLANA) {
    return;
  }
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    return;
  }
  if (IsAccountConnected(*account) &&
      !delegate_->IsAccountAllowed(mojom::CoinType::SOL, account->address)) {
    Disconnect();
  }
}

void SolanaProviderImpl::SignMessage(
    const std::vector<uint8_t>& blob_msg,
    const std::optional<std::string>& display_encoding,
    SignMessageCallback callback) {
  auto account = keyring_service_->GetSelectedSolanaDappAccount();
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            base::Value::Dict());
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            base::Value::Dict());
    return;
  }
  // Prevent transaction payload from being signed
  if (SolanaMessage::Deserialize(blob_msg)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            base::Value::Dict());
    return;
  }
  std::string message;
  if (display_encoding && *display_encoding == "hex") {
    message = base::StrCat({"0x", base::HexEncode(blob_msg)});
  } else {
    message = std::string(blob_msg.begin(), blob_msg.end());
  }
  auto request = mojom::SignMessageRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, account->account_id.Clone(),
      mojom::SignDataUnion::NewSolanaSignData(
          mojom::SolanaSignData::New(message, blob_msg)),
      mojom::CoinType::SOL,
      json_rpc_service_->GetChainIdSync(mojom::CoinType::SOL,
                                        delegate_->GetOrigin()));

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignMessageRequestProcessed,
                     weak_factory_.GetWeakPtr(), blob_msg, std::move(account),
                     std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::Request(base::Value::Dict arg,
                                 RequestCallback callback) {
  const std::string* method = arg.FindString("method");
  if (!method) {
    std::move(callback).Run(mojom::SolanaProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
                            base::Value::Dict());
    return;
  }
  base::Value::Dict* params = arg.FindDict("params");

  // params is optional for connect and disconnect doesn't need it
  if (!params && (*method == solana::kSignTransaction ||
                  *method == solana::kSignAndSendTransaction ||
                  *method == solana::kSignAllTransactions ||
                  *method == solana::kSignMessage)) {
    std::move(callback).Run(mojom::SolanaProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
                            base::Value::Dict());
    return;
  }

  if (*method == solana::kConnect) {
    std::optional<base::Value::Dict> option = std::nullopt;
    if (params) {
      option = std::move(*params);
    }
    Connect(std::move(option),
            base::BindOnce(&SolanaProviderImpl::OnRequestConnect,
                           weak_factory_.GetWeakPtr(), std::move(callback)));
  } else if (*method == solana::kDisconnect) {
    Disconnect();
    std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                            base::Value::Dict());
  } else if (*method == solana::kSignTransaction) {
    const std::string* message = params->FindString(kMessage);
    if (!message) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kParsingError,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
          base::Value::Dict());
      return;
    }
    SignTransaction(
        mojom::SolanaSignTransactionParam::New(
            *message, std::vector<mojom::SignaturePubkeyPairPtr>()),
        base::BindOnce(&SolanaProviderImpl::OnRequestSignTransaction,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
  } else if (*method == solana::kSignAndSendTransaction) {
    const std::string* message = params->FindString(kMessage);
    if (!message) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kParsingError,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
          base::Value::Dict());
      return;
    }
    base::Value::Dict* options_dict = params->FindDict(kOptions);
    std::optional<base::Value::Dict> options = std::nullopt;
    if (options_dict) {
      options = std::move(*options_dict);
    }
    SignAndSendTransaction(
        mojom::SolanaSignTransactionParam::New(
            *message, std::vector<mojom::SignaturePubkeyPairPtr>()),
        std::move(options), std::move(callback));
  } else if (*method == solana::kSignAllTransactions) {
    const base::Value::List* messages = params->FindList(kMessage);
    if (!messages) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kParsingError,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
          base::Value::Dict());
      return;
    }

    std::vector<mojom::SolanaSignTransactionParamPtr> sign_params;
    for (const auto& message : *messages) {
      auto param = mojom::SolanaSignTransactionParam::New();
      const std::string* encoded_serialized_msg = message.GetIfString();
      if (encoded_serialized_msg) {
        param->encoded_serialized_msg = *encoded_serialized_msg;
      }
      sign_params.push_back(std::move(param));
    }
    SignAllTransactions(
        std::move(sign_params),
        base::BindOnce(&SolanaProviderImpl::OnRequestSignAllTransactions,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
  } else if (*method == solana::kSignMessage) {
    const auto* message = params->FindBlob(kMessage);
    if (!message) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kParsingError,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
          base::Value::Dict());
      return;
    }
    const std::string* display_str = params->FindString("display");
    std::optional<std::string> display = std::nullopt;
    if (display_str) {
      display = *display_str;
    }
    SignMessage(*message, std::move(display), std::move(callback));
  } else {
    std::move(callback).Run(
        mojom::SolanaProviderError::kMethodNotFound,
        l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR),
        base::Value::Dict());
  }
}

bool SolanaProviderImpl::IsAccountConnected(const mojom::AccountInfo& account) {
  return delegate_->IsSolanaAccountConnected(account.address);
}

void SolanaProviderImpl::OnConnect(
    const std::vector<mojom::AccountInfoPtr>& requested_accounts,
    ConnectCallback callback,
    RequestPermissionsError error,
    const std::optional<std::vector<std::string>>& allowed_accounts) {
  if (error == RequestPermissionsError::kInternal) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            "");
  } else if (error == RequestPermissionsError::kRequestInProgress) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kResourceUnavailable,
        l10n_util::GetStringUTF8(
            IDS_WALLET_REQUESTED_RESOURCE_NOT_AVAILABLE_ERROR),
        "");
  } else if (error == RequestPermissionsError::kNone) {
    CHECK(allowed_accounts);
    if (allowed_accounts->size()) {
      // There should be only one account to be connected
      CHECK_EQ(allowed_accounts->size(), 1u);
      auto account = keyring_service_->GetSelectedSolanaDappAccount();
      // Set connected account to be selected if connected one and selected
      // account are different.
      const std::string& allowed_account_address = allowed_accounts->at(0);
      if (account && account->address != allowed_account_address) {
        auto account_it = base::ranges::find_if(
            requested_accounts, [&allowed_account_address](
                                    const mojom::AccountInfoPtr& account_info) {
              return account_info->address == allowed_account_address;
            });
        CHECK(account_it != requested_accounts.end());
        // SetSelectedAccount result shouldn't affect connect()
        keyring_service_->SetSelectedAccountSync(
            (*account_it)->account_id.Clone());
      }
      std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                              allowed_account_address);
      delegate_->AddSolanaConnectedAccount(allowed_account_address);
    } else {
      std::move(callback).Run(
          mojom::SolanaProviderError::kUserRejectedRequest,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
    }
  } else {
    NOTREACHED_IN_MIGRATION();
  }
}

void SolanaProviderImpl::OnSignMessageRequestProcessed(
    const std::vector<uint8_t>& blob_msg,
    const mojom::AccountInfoPtr& account,
    SignMessageCallback callback,
    bool approved,
    mojom::EthereumSignatureBytesPtr hw_signature,
    const std::optional<std::string>& error) {
  base::Value::Dict result;
  if (error && !error->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError, *error,
                            std::move(result));
    return;
  }

  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::move(result));
    return;
  }

  std::optional<std::vector<uint8_t>> sig_bytes;
  if (account->account_id->kind != mojom::AccountKind::kHardware) {
    sig_bytes = keyring_service_->SignMessageBySolanaKeyring(
        account->account_id, blob_msg);
  } else if (hw_signature) {
    // TODO(apaymyshev): Hardware signing of solana messages is not supported
    // yet by UI.
    sig_bytes = std::move(hw_signature->bytes);
  }

  if (!sig_bytes || sig_bytes->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::move(result));
    return;
  }

  result.Set(kPublicKey, account->address);
  result.Set(kSignature, Base58Encode(*sig_bytes));

  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                          std::move(result));
}

void SolanaProviderImpl::OnRequestConnect(RequestCallback callback,
                                          mojom::SolanaProviderError error,
                                          const std::string& error_message,
                                          const std::string& public_key) {
  base::Value::Dict result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    result.Set(kPublicKey, public_key);
  }
  std::move(callback).Run(error, error_message, std::move(result));
}

void SolanaProviderImpl::OnRequestSignTransaction(
    RequestCallback callback,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    const std::vector<uint8_t>& serialized_tx,
    mojom::SolanaMessageVersion version) {
  base::Value::Dict result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    auto tx = SolanaTransaction::FromSignedTransactionBytes(serialized_tx);
    DCHECK(tx);
    result.Set(kPublicKey, tx->message()->fee_payer());
    result.Set(kSignature, Base58Encode(tx->raw_signatures()));
  }
  std::move(callback).Run(error, error_message, std::move(result));
}

void SolanaProviderImpl::OnRequestSignAllTransactions(
    RequestCallback callback,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    const std::vector<std::vector<uint8_t>>& serialized_txs,
    const std::vector<mojom::SolanaMessageVersion>& versions) {
  base::Value::Dict result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    base::Value::List signatures;
    for (const auto& serialized_tx : serialized_txs) {
      auto tx = SolanaTransaction::FromSignedTransactionBytes(serialized_tx);
      DCHECK(tx);
      if (!result.contains(kPublicKey)) {
        // The API expect only one signer (the selected account) from all
        // transactions in the array.
        result.Set(kPublicKey, tx->message()->fee_payer());
      }
      signatures.Append(Base58Encode(tx->raw_signatures()));
    }
    result.Set(kSignature, std::move(signatures));
  }
  std::move(callback).Run(error, error_message, std::move(result));
}

void SolanaProviderImpl::Unlocked() {
  if (pending_connect_callback_) {
    Connect(std::move(pending_connect_arg_),
            std::move(pending_connect_callback_));
  }
}

void SolanaProviderImpl::SelectedDappAccountChanged(
    mojom::CoinType coin,
    mojom::AccountInfoPtr account) {
  if (!events_listener_.is_bound() || coin != mojom::CoinType::SOL) {
    return;
  }

  if (account && IsAccountConnected(*account)) {
    events_listener_->AccountChangedEvent(account->address);
  } else {
    events_listener_->AccountChangedEvent(std::nullopt);
  }
}

}  // namespace brave_wallet
