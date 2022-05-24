/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_provider_impl.h"

#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// When onlyIfTrusted is true, the request would be rejected when selected
// account doesn't have permission.
constexpr char kOnlyIfTrustedOption[] = "onlyIfTrusted";

}  // namespace

SolanaProviderImpl::SolanaProviderImpl(
    KeyringService* keyring_service,
    BraveWalletService* brave_wallet_service,
    TxService* tx_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate)
    : keyring_service_(keyring_service),
      brave_wallet_service_(brave_wallet_service),
      tx_service_(tx_service),
      delegate_(std::move(delegate)),
      weak_factory_(this) {
  DCHECK(keyring_service_);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(tx_service);
  tx_service_->AddObserver(tx_observer_receiver_.BindNewPipeAndPassRemote());
}

SolanaProviderImpl::~SolanaProviderImpl() = default;

void SolanaProviderImpl::Init(
    ::mojo::PendingRemote<mojom::SolanaEventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
  }
}

void SolanaProviderImpl::Connect(absl::optional<base::Value> arg,
                                 ConnectCallback callback) {
  DCHECK(delegate_);
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            "");
    return;
  }

  bool is_eagerly_connect = false;
  if (arg.has_value()) {
    const base::Value::Dict* arg_dict = arg->GetIfDict();
    if (arg_dict) {
      absl::optional<bool> only_if_trusted =
          arg_dict->FindBool(kOnlyIfTrustedOption);
      if (only_if_trusted.has_value() && *only_if_trusted) {
        is_eagerly_connect = true;
      }
    }
  }

  delegate_->IsAccountAllowed(
      mojom::CoinType::SOL, *account,
      base::BindOnce(&SolanaProviderImpl::ContinueConnect,
                     weak_factory_.GetWeakPtr(), is_eagerly_connect, *account,
                     std::move(callback)));
}

void SolanaProviderImpl::Disconnect() {
  DCHECK(keyring_service_);
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (account)
    connected_set_.erase(*account);
}

void SolanaProviderImpl::IsConnected(IsConnectedCallback callback) {
  DCHECK(keyring_service_);
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account)
    std::move(callback).Run(false);
  else
    std::move(callback).Run(IsAccountConnected(*account));
}

void SolanaProviderImpl::GetPublicKey(GetPublicKeyCallback callback) {
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account) {
    std::move(callback).Run("");
    return;
  }
  if (IsAccountConnected(*account))
    std::move(callback).Run(*account);
  else
    std::move(callback).Run("");
}

absl::optional<SolanaMessage> SolanaProviderImpl::GetDeserializedMessage(
    const std::string& encoded_serialized_msg,
    const std::string& account) {
  std::vector<uint8_t> message_bytes;
  if (!Base58Decode(encoded_serialized_msg, &message_bytes, kSolanaMaxTxSize,
                    false)) {
    return absl::nullopt;
  }

  auto msg = SolanaMessage::Deserialize(message_bytes);
  if (!msg) {
    return absl::nullopt;
  }

  // Sanity check after deserialization:
  // 1. Fee payer should be the current selected account.
  // 2. Serialize the created message object and encode in Base58 will be the
  //    same as original encoded message.
  // 3. Only one signer (the fee payer) is allowed, we do not support multisig
  //    for dApp requests currently.
  if (account != msg->fee_payer())
    return absl::nullopt;
  std::vector<std::string> signers;
  auto serialized_message = msg->Serialize(&signers);
  if (!serialized_message ||
      Base58Encode(*serialized_message) != encoded_serialized_msg ||
      signers.size() != 1) {
    return absl::nullopt;
  }

  return msg;
}

void SolanaProviderImpl::SignTransaction(
    const std::string& encoded_serialized_msg,
    SignTransactionCallback callback) {
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>());
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            std::vector<uint8_t>());
    return;
  }
  auto msg = GetDeserializedMessage(encoded_serialized_msg, *account);
  if (!msg) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>());
    return;
  }

  SolanaTransaction tx = SolanaTransaction(std::move(*msg));
  tx.set_tx_type(mojom::TransactionType::SolanaDappSignTransaction);
  auto request = mojom::SignTransactionRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, *account,
      mojom::TxDataUnion::NewSolanaTxData(tx.ToSolanaTxData()));
  brave_wallet_service_->AddSignTransactionRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignTransactionRequestProcessed,
                     weak_factory_.GetWeakPtr(), tx, std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::OnSignTransactionRequestProcessed(
    const SolanaTransaction& tx,
    SignTransactionCallback callback,
    bool approved) {
  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::vector<uint8_t>());
    return;
  }

  auto signed_tx = tx.GetSignedTransactionBytes(keyring_service_);
  if (!signed_tx) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "", *signed_tx);
}

void SolanaProviderImpl::SignAllTransactions(
    const std::vector<std::string>& encoded_serialized_msgs,
    SignAllTransactionsCallback callback) {
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<std::vector<uint8_t>>());
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            std::vector<std::vector<uint8_t>>());
    return;
  }

  std::vector<mojom::TxDataUnionPtr> tx_datas;
  std::vector<SolanaTransaction> txs;
  for (const auto& encoded_serialized_msg : encoded_serialized_msgs) {
    auto msg = GetDeserializedMessage(encoded_serialized_msg, *account);
    if (!msg) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
          std::vector<std::vector<uint8_t>>());
      return;
    }

    SolanaTransaction tx = SolanaTransaction(std::move(*msg));
    tx.set_tx_type(mojom::TransactionType::SolanaDappSignTransaction);
    tx_datas.push_back(
        mojom::TxDataUnion::NewSolanaTxData(tx.ToSolanaTxData()));
    txs.push_back(std::move(tx));
  }

  auto request = mojom::SignAllTransactionsRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, *account,
      std::move(tx_datas));

  brave_wallet_service_->AddSignAllTransactionsRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignAllTransactionsRequestProcessed,
                     weak_factory_.GetWeakPtr(), txs, std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::OnSignAllTransactionsRequestProcessed(
    const std::vector<SolanaTransaction>& txs,
    SignAllTransactionsCallback callback,
    bool approved) {
  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::vector<std::vector<uint8_t>>());
    return;
  }

  std::vector<std::vector<uint8_t>> signed_txs;
  for (const auto& tx : txs) {
    auto signed_tx = tx.GetSignedTransactionBytes(keyring_service_);
    if (!signed_tx) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
          std::vector<std::vector<uint8_t>>());
      return;
    }
    signed_txs.push_back(std::move(*signed_tx));
  }

  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "", signed_txs);
}

void SolanaProviderImpl::SignAndSendTransaction(
    const std::string& encoded_serialized_msg,
    SignAndSendTransactionCallback callback) {
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            base::Value(base::Value::Type::DICTIONARY));
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            base::Value(base::Value::Type::DICTIONARY));
    return;
  }

  auto msg = GetDeserializedMessage(encoded_serialized_msg, *account);
  if (!msg) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            base::Value(base::Value::Type::DICTIONARY));
    return;
  }

  SolanaTransaction tx = SolanaTransaction(std::move(*msg));
  tx.set_tx_type(mojom::TransactionType::SolanaDappSignAndSendTransaction);
  tx_service_->AddUnapprovedTransaction(
      mojom::TxDataUnion::NewSolanaTxData(tx.ToSolanaTxData()),
      tx.message()->fee_payer(), delegate_->GetOrigin(),
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
                            base::Value(base::Value::Type::DICTIONARY));
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
      tx_status != mojom::TransactionStatus::Error)
    return;

  std::string tx_meta_id = tx_info->id;
  if (!sign_and_send_tx_callbacks_.contains(tx_meta_id))
    return;

  auto callback = std::move(sign_and_send_tx_callbacks_[tx_meta_id]);
  base::Value result(base::Value::Type::DICTIONARY);
  if (tx_status == mojom::TransactionStatus::Submitted) {
    result.SetStringKey("publicKey", tx_info->from_address);
    result.SetStringKey("signature", tx_info->tx_hash);
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
    NOTREACHED();
  }
  sign_and_send_tx_callbacks_.erase(tx_meta_id);
}

void SolanaProviderImpl::SignMessage(
    const std::vector<uint8_t>& blob_msg,
    const absl::optional<std::string>& display_encoding,
    SignMessageCallback callback) {
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            base::Value(base::Value::Type::DICTIONARY));
    return;
  }
  if (!IsAccountConnected(*account)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            base::Value(base::Value::Type::DICTIONARY));
    return;
  }
  // Prevent transaction payload from being signed
  if (SolanaMessage::Deserialize(blob_msg)) {
    std::move(callback).Run(mojom::SolanaProviderError::kUnauthorized,
                            l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED),
                            base::Value(base::Value::Type::DICTIONARY));
    return;
  }
  std::string message;
  if (display_encoding && *display_encoding == "hex")
    message = base::StrCat({"0x", base::HexEncode(blob_msg)});
  else
    message = std::string(blob_msg.begin(), blob_msg.end());
  auto request = mojom::SignMessageRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, *account, message, false,
      absl::nullopt, absl::nullopt, mojom::CoinType::SOL);

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignMessageRequestProcessed,
                     weak_factory_.GetWeakPtr(), blob_msg, *account,
                     std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::Request(base::Value arg, RequestCallback callback) {
  base::Value result(base::Value::Type::DICTIONARY);
  NOTIMPLEMENTED();
  std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
                          std::move(result));
}

bool SolanaProviderImpl::IsAccountConnected(const std::string& account) {
  return connected_set_.contains(account);
}

void SolanaProviderImpl::ContinueConnect(bool is_eagerly_connect,
                                         const std::string& selected_account,
                                         ConnectCallback callback,
                                         bool is_selected_account_allowed) {
  if (is_selected_account_allowed) {
    std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                            selected_account);
    connected_set_.insert(selected_account);
  } else if (!is_selected_account_allowed && is_eagerly_connect) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
  } else {
    delegate_->RequestPermissions(
        mojom::CoinType::SOL, {selected_account},
        base::BindOnce(&SolanaProviderImpl::OnConnect,
                       weak_factory_.GetWeakPtr(), selected_account,
                       std::move(callback)));
  }
}

void SolanaProviderImpl::OnConnect(
    const std::string& requested_account,
    ConnectCallback callback,
    RequestPermissionsError error,
    const absl::optional<std::vector<std::string>>& allowed_accounts) {
  if (error == RequestPermissionsError::kInternal) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            "");
  } else if (error == RequestPermissionsError::kRequestInProgress) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
  } else if (error == RequestPermissionsError::kNone) {
    CHECK(allowed_accounts);
    if (allowed_accounts->size()) {
      std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                              allowed_accounts->at(0));
      connected_set_.insert(allowed_accounts->at(0));
    } else {
      std::move(callback).Run(
          mojom::SolanaProviderError::kUserRejectedRequest,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
    }
  } else {
    NOTREACHED();
  }
}

void SolanaProviderImpl::OnSignMessageRequestProcessed(
    const std::vector<uint8_t>& blob_msg,
    const std::string& account,
    SignMessageCallback callback,
    bool approved,
    const std::string& signature_not_used,
    const std::string& error_not_used) {
  base::Value result(base::Value::Type::DICTIONARY);
  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::move(result));
  } else {
    auto signature = keyring_service_->SignMessage(mojom::kSolanaKeyringId,
                                                   account, blob_msg);
    result.GetDict().Set("publicKey", account);
    result.GetDict().Set("signature", Base58Encode(signature));

    std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                            std::move(result));
  }
}

void SolanaProviderImpl::SelectedAccountChanged(mojom::CoinType coin) {
  if (!events_listener_.is_bound() || coin != mojom::CoinType::SOL)
    return;

  DCHECK(keyring_service_);
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (IsAccountConnected(*account))
    events_listener_->AccountChangedEvent(account);
  else
    events_listener_->AccountChangedEvent(absl::nullopt);
}

}  // namespace brave_wallet
