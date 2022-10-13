/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_provider_impl.h"

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

void SolanaProviderImpl::Connect(absl::optional<base::Value::Dict> arg,
                                 ConnectCallback callback) {
  DCHECK(delegate_);
  if (delegate_->IsPermissionDenied(mojom::CoinType::SOL)) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), "");
    return;
  }
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (!account) {
    // Prompt users to create a Solana account. If wallet is not setup, users
    // will be lead to onboarding first.
    delegate_->ShowAccountCreation(mojom::CoinType::SOL);
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            "");
    return;
  }

  bool is_eagerly_connect = false;
  if (arg.has_value()) {
    is_eagerly_connect = arg->FindBool(kOnlyIfTrustedOption).value_or(false);
  }

  if (keyring_service_->IsLocked(mojom::kSolanaKeyringId)) {
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
    delegate_->RemoveSolanaConnectedAccount(*account);
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

absl::optional<std::pair<SolanaMessage, std::vector<uint8_t>>>
SolanaProviderImpl::GetDeserializedMessage(
    const std::string& encoded_serialized_msg,
    const std::string& account) {
  std::vector<uint8_t> message_bytes;
  if (!Base58Decode(encoded_serialized_msg, &message_bytes, kSolanaMaxTxSize,
                    false)) {
    return absl::nullopt;
  }

  auto msg = SolanaMessage::Deserialize(message_bytes);
  if (!msg)
    return absl::nullopt;

  // Sanity check after deserialization:
  //   - Fee payer should be the current selected account.
  // Note: We cannot check Base58Encode(msg->Serialize()) is equal to the
  // original encoded message passed in because the order of accounts with the
  // same is_signer and is_writable value can be different between different
  // implementation. See https://github.com/brave/brave-browser/issues/23542
  // for more details.
  if (account != msg->fee_payer())
    return absl::nullopt;

  return std::make_pair(std::move(*msg), std::move(message_bytes));
}

void SolanaProviderImpl::SignTransaction(
    mojom::SolanaSignTransactionParamPtr param,
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
  auto msg_pair =
      GetDeserializedMessage(param->encoded_serialized_msg, *account);
  if (!msg_pair) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>());
    return;
  }

  auto tx = std::make_unique<SolanaTransaction>(std::move(msg_pair->first),
                                                std::move(param));
  tx->set_tx_type(mojom::TransactionType::SolanaDappSignTransaction);
  auto request = mojom::SignTransactionRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, *account,
      mojom::TxDataUnion::NewSolanaTxData(tx->ToSolanaTxData()),
      mojom::ByteArrayStringUnion::NewBytes(std::move(msg_pair->second)),
      mojom::CoinType::SOL);
  brave_wallet_service_->AddSignTransactionRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignTransactionRequestProcessed,
                     weak_factory_.GetWeakPtr(), std::move(tx), *account,
                     std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::OnSignTransactionRequestProcessed(
    std::unique_ptr<SolanaTransaction> tx,
    const std::string& account,
    SignTransactionCallback callback,
    bool approved,
    mojom::ByteArrayStringUnionPtr signature,
    const absl::optional<std::string>& error) {
  if (error && !error->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError, *error,
                            std::vector<uint8_t>());
    return;
  }

  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::vector<uint8_t>());
    return;
  }

  absl::optional<std::vector<uint8_t>> signed_tx;
  if (!keyring_service_->IsHardwareAccount(account)) {
    signed_tx = tx->GetSignedTransactionBytes(keyring_service_);
  } else if (signature && signature->is_bytes()) {  // hardware
    signed_tx = tx->GetSignedTransactionBytes(keyring_service_,
                                              &signature->get_bytes());
  }

  if (!signed_tx || signed_tx->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "", *signed_tx);
}

void SolanaProviderImpl::SignAllTransactions(
    std::vector<mojom::SolanaSignTransactionParamPtr> params,
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
  std::vector<std::unique_ptr<SolanaTransaction>> txs;
  std::vector<mojom::ByteArrayStringUnionPtr> raw_messages;
  for (auto& param : params) {
    auto msg_pair =
        GetDeserializedMessage(param->encoded_serialized_msg, *account);
    if (!msg_pair) {
      std::move(callback).Run(
          mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
          std::vector<std::vector<uint8_t>>());
      return;
    }

    auto tx = std::make_unique<SolanaTransaction>(std::move(msg_pair->first),
                                                  std::move(param));
    tx->set_tx_type(mojom::TransactionType::SolanaDappSignTransaction);
    raw_messages.push_back(
        mojom::ByteArrayStringUnion::NewBytes(std::move(msg_pair->second)));
    tx_datas.push_back(
        mojom::TxDataUnion::NewSolanaTxData(tx->ToSolanaTxData()));
    txs.push_back(std::move(tx));
  }

  auto request = mojom::SignAllTransactionsRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, *account, std::move(tx_datas),
      std::move(raw_messages), mojom::CoinType::SOL);

  brave_wallet_service_->AddSignAllTransactionsRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignAllTransactionsRequestProcessed,
                     weak_factory_.GetWeakPtr(), std::move(txs), *account,
                     std::move(callback)));
  delegate_->ShowPanel();
}

void SolanaProviderImpl::OnSignAllTransactionsRequestProcessed(
    const std::vector<std::unique_ptr<SolanaTransaction>>& txs,
    const std::string& account,
    SignAllTransactionsCallback callback,
    bool approved,
    absl::optional<std::vector<mojom::ByteArrayStringUnionPtr>> signatures,
    const absl::optional<std::string>& error) {
  if (error && !error->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError, *error,
                            std::vector<std::vector<uint8_t>>());
    return;
  }

  if (!approved) {
    std::move(callback).Run(
        mojom::SolanaProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
        std::vector<std::vector<uint8_t>>());
    return;
  }

  bool is_hardware_account = keyring_service_->IsHardwareAccount(account);
  if (is_hardware_account &&
      (!signatures || signatures->size() != txs.size())) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::vector<std::vector<uint8_t>>());
    return;
  }

  std::vector<std::vector<uint8_t>> signed_txs;
  for (size_t i = 0; i < txs.size(); ++i) {
    absl::optional<std::vector<uint8_t>> signed_tx;
    if (!is_hardware_account) {
      signed_tx = txs[i]->GetSignedTransactionBytes(keyring_service_);
    } else if (signatures->at(i) && signatures->at(i)->is_bytes()) {
      signed_tx = txs[i]->GetSignedTransactionBytes(
          keyring_service_, &signatures->at(i)->get_bytes());
    }

    if (!signed_tx || signed_tx->empty()) {
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
    mojom::SolanaSignTransactionParamPtr param,
    absl::optional<base::Value::Dict> send_options,
    SignAndSendTransactionCallback callback) {
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
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

  auto msg_pair =
      GetDeserializedMessage(param->encoded_serialized_msg, *account);
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

  tx_service_->AddUnapprovedTransaction(
      mojom::TxDataUnion::NewSolanaTxData(tx.ToSolanaTxData()),
      tx.message()->fee_payer(), delegate_->GetOrigin(), absl::nullopt,
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
      tx_status != mojom::TransactionStatus::Error)
    return;

  std::string tx_meta_id = tx_info->id;
  if (!sign_and_send_tx_callbacks_.contains(tx_meta_id))
    return;

  auto callback = std::move(sign_and_send_tx_callbacks_[tx_meta_id]);
  base::Value::Dict result;
  if (tx_status == mojom::TransactionStatus::Submitted) {
    result.Set(kPublicKey, tx_info->from_address);
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
  if (display_encoding && *display_encoding == "hex")
    message = base::StrCat({"0x", base::HexEncode(blob_msg)});
  else
    message = std::string(blob_msg.begin(), blob_msg.end());
  auto request = mojom::SignMessageRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), -1, *account, message, false,
      absl::nullopt, absl::nullopt, blob_msg, mojom::CoinType::SOL);

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&SolanaProviderImpl::OnSignMessageRequestProcessed,
                     weak_factory_.GetWeakPtr(), blob_msg, *account,
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
    absl::optional<base::Value::Dict> option = absl::nullopt;
    if (params)
      option = std::move(*params);
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
    absl::optional<base::Value::Dict> options = absl::nullopt;
    if (options_dict)
      options = std::move(*options_dict);
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
      if (encoded_serialized_msg)
        param->encoded_serialized_msg = *encoded_serialized_msg;
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
    absl::optional<std::string> display = absl::nullopt;
    if (display_str)
      display = *display_str;
    SignMessage(*message, std::move(display), std::move(callback));
  } else {
    std::move(callback).Run(
        mojom::SolanaProviderError::kMethodNotFound,
        l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR),
        base::Value::Dict());
  }
}

bool SolanaProviderImpl::IsAccountConnected(const std::string& account) {
  return delegate_->IsSolanaAccountConnected(account);
}

void SolanaProviderImpl::ContinueConnect(bool is_eagerly_connect,
                                         const std::string& selected_account,
                                         ConnectCallback callback,
                                         bool is_selected_account_allowed) {
  if (is_selected_account_allowed) {
    std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                            selected_account);
    delegate_->AddSolanaConnectedAccount(selected_account);
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
      delegate_->AddSolanaConnectedAccount(allowed_accounts->at(0));
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
    mojom::ByteArrayStringUnionPtr signature,
    const absl::optional<std::string>& error) {
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

  bool is_hardware_account = keyring_service_->IsHardwareAccount(account);
  absl::optional<std::vector<uint8_t>> sig_bytes;
  if (!is_hardware_account) {
    sig_bytes = keyring_service_->SignMessage(mojom::kSolanaKeyringId, account,
                                              blob_msg);
  } else if (signature && signature->is_bytes()) {
    sig_bytes = signature->get_bytes();
  }

  if (!sig_bytes || sig_bytes->empty()) {
    std::move(callback).Run(mojom::SolanaProviderError::kInternalError,
                            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR),
                            std::move(result));
    return;
  }

  result.Set(kPublicKey, account);
  result.Set(kSignature, Base58Encode(*sig_bytes));

  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                          std::move(result));
}

void SolanaProviderImpl::OnRequestConnect(RequestCallback callback,
                                          mojom::SolanaProviderError error,
                                          const std::string& error_message,
                                          const std::string& public_key) {
  base::Value::Dict result;
  if (error == mojom::SolanaProviderError::kSuccess)
    result.Set(kPublicKey, public_key);
  std::move(callback).Run(error, error_message, std::move(result));
}

void SolanaProviderImpl::OnRequestSignTransaction(
    RequestCallback callback,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    const std::vector<uint8_t>& serialized_tx) {
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
    const std::vector<std::vector<uint8_t>>& serialized_txs) {
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

void SolanaProviderImpl::SelectedAccountChanged(mojom::CoinType coin) {
  if (!events_listener_.is_bound() || coin != mojom::CoinType::SOL)
    return;

  DCHECK(keyring_service_);
  absl::optional<std::string> account =
      keyring_service_->GetSelectedAccount(mojom::CoinType::SOL);
  if (account && IsAccountConnected(*account))
    events_listener_->AccountChangedEvent(account);
  else
    events_listener_->AccountChangedEvent(absl::nullopt);
}

}  // namespace brave_wallet
