/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_transaction.h"

#include <optional>

#include "base/base64.h"
#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace brave_wallet {

namespace {

constexpr char kSendOptions[] = "send_options";
constexpr char kPublicKey[] = "public_key";
constexpr char kSignature[] = "signature";
constexpr char kSignatures[] = "signatures";
constexpr char kSignTxParam[] = "sign_tx_param";
constexpr char kEncodedSerializedMsg[] = "encoded_serialized_msg";

// Below are using camel cases so we can handle the parameters from dApp
// requests directly with the same key.
constexpr char kMaxRetries[] = "maxRetries";
constexpr char kPreflightCommitment[] = "preflightCommitment";
constexpr char kSkipPreflight[] = "skipPreflight";

}  // namespace

SolanaTransaction::SendOptions::SendOptions() = default;
SolanaTransaction::SendOptions::~SendOptions() = default;
SolanaTransaction::SendOptions::SendOptions(
    const SolanaTransaction::SendOptions&) = default;
SolanaTransaction::SendOptions::SendOptions(
    std::optional<uint64_t> max_retries_param,
    std::optional<std::string> preflight_commitment_param,
    std::optional<bool> skip_preflight_param)
    : max_retries(std::move(max_retries_param)),
      preflight_commitment(std::move(preflight_commitment_param)),
      skip_preflight(std::move(skip_preflight_param)) {}

bool SolanaTransaction::SendOptions::operator==(
    const SolanaTransaction::SendOptions& options) const {
  return max_retries == options.max_retries &&
         preflight_commitment == options.preflight_commitment &&
         skip_preflight == options.skip_preflight;
}

bool SolanaTransaction::SendOptions::operator!=(
    const SolanaTransaction::SendOptions& options) const {
  return !operator==(options);
}

// static
std::optional<SolanaTransaction::SendOptions>
SolanaTransaction::SendOptions::FromValue(
    std::optional<base::Value::Dict> value) {
  if (!value) {
    return std::nullopt;
  }
  return FromValue(*value);
}

// static
std::optional<SolanaTransaction::SendOptions>
SolanaTransaction::SendOptions::FromValue(const base::Value::Dict& dict) {
  SolanaTransaction::SendOptions options;

  if (auto* max_retries_string = dict.FindString(kMaxRetries)) {
    // Type of maxRetries is string when it's from preference values.
    uint64_t max_retries = 0;
    if (base::StringToUint64(*max_retries_string, &max_retries)) {
      options.max_retries = max_retries;
    }
  } else if (auto max_retries_number = dict.FindDouble(kMaxRetries)) {
    // Type of maxRetries is number when it's from dApp requests.
    // We cap the maximum to 2^53-1 here for double precision, it's safe here
    // because it does not make sense for dApps to set maxRetries that large.
    if (*max_retries_number >= 0 &&
        *max_retries_number <= static_cast<double>(kMaxSafeIntegerUint64)) {
      options.max_retries = max_retries_number;
    }
  }

  auto* commitment = dict.FindString(kPreflightCommitment);
  if (commitment && IsValidCommitmentString(*commitment)) {
    options.preflight_commitment = *commitment;
  }
  options.skip_preflight = dict.FindBool(kSkipPreflight);

  return options;
}

base::Value::Dict SolanaTransaction::SendOptions::ToValue() const {
  base::Value::Dict options;
  if (max_retries) {
    options.Set(kMaxRetries, base::NumberToString(*max_retries));
  }
  if (preflight_commitment) {
    options.Set(kPreflightCommitment, *preflight_commitment);
  }
  if (skip_preflight) {
    options.Set(kSkipPreflight, *skip_preflight);
  }
  return options;
}

// static
std::optional<SolanaTransaction::SendOptions>
SolanaTransaction::SendOptions::FromMojomSendOptions(
    mojom::SolanaSendTransactionOptionsPtr mojom_options) {
  if (!mojom_options) {
    return std::nullopt;
  }

  SendOptions options;
  if (mojom_options->max_retries) {
    options.max_retries = mojom_options->max_retries->max_retries;
  }
  if (mojom_options->skip_preflight) {
    options.skip_preflight = mojom_options->skip_preflight->skip_preflight;
  }
  if (mojom_options->preflight_commitment &&
      IsValidCommitmentString(*mojom_options->preflight_commitment)) {
    options.preflight_commitment = mojom_options->preflight_commitment;
  }
  return options;
}

mojom::SolanaSendTransactionOptionsPtr
SolanaTransaction::SendOptions::ToMojomSendOptions() const {
  auto send_options = mojom::SolanaSendTransactionOptions::New();
  if (max_retries) {
    send_options->max_retries = mojom::OptionalMaxRetries::New(*max_retries);
  }
  if (preflight_commitment) {
    send_options->preflight_commitment = *preflight_commitment;
  }
  if (skip_preflight) {
    send_options->skip_preflight =
        mojom::OptionalSkipPreflight::New(*skip_preflight);
  }
  return send_options;
}

SolanaTransaction::SolanaTransaction(
    mojom::SolanaMessageVersion version,
    const std::string& recent_blockhash,
    uint64_t last_valid_block_height,
    const std::string& fee_payer,
    const SolanaMessageHeader& message_header,
    std::vector<SolanaAddress> static_account_keys,
    std::vector<SolanaInstruction> instructions,
    std::vector<SolanaMessageAddressTableLookup> addr_table_lookups)
    : message_(version,
               recent_blockhash,
               last_valid_block_height,
               fee_payer,
               message_header,
               std::move(static_account_keys),
               std::move(instructions),
               std::move(addr_table_lookups)) {}

SolanaTransaction::SolanaTransaction(SolanaMessage message)
    : message_(std::move(message)) {}

SolanaTransaction::SolanaTransaction(SolanaMessage message,
                                     std::vector<uint8_t> raw_signatures)
    : message_(std::move(message)),
      raw_signatures_(std::move(raw_signatures)) {}

SolanaTransaction::SolanaTransaction(
    SolanaMessage message,
    mojom::SolanaSignTransactionParamPtr sign_tx_param)
    : message_(std::move(message)), sign_tx_param_(std::move(sign_tx_param)) {}

SolanaTransaction::~SolanaTransaction() = default;

bool SolanaTransaction::operator==(const SolanaTransaction& tx) const {
  return message_ == tx.message_ && raw_signatures_ == tx.raw_signatures_ &&
         wired_tx_ == tx.wired_tx_ && sign_tx_param_ == tx.sign_tx_param_ &&
         to_wallet_address_ == tx.to_wallet_address_ &&
         token_address_ == tx.token_address_ && tx_type_ == tx.tx_type_ &&
         lamports_ == tx.lamports_ && amount_ == tx.amount_ &&
         send_options_ == tx.send_options_ &&
         tx.fee_estimation_ == fee_estimation_;
}

bool SolanaTransaction::operator!=(const SolanaTransaction& tx) const {
  return !operator==(tx);
}

// Get serialized message bytes and array of signers.
// Serialized message will be the result of decoding
// sign_tx_param_->encoded_serialized_msg if sign_tx_param_ exists.
std::optional<std::pair<std::vector<uint8_t>, std::vector<std::string>>>
SolanaTransaction::GetSerializedMessage() const {
  if (!sign_tx_param_) {
    std::vector<std::string> signers;
    auto message_bytes = message_.Serialize(&signers);
    if (!message_bytes || signers.empty()) {
      return std::nullopt;
    }

    return std::make_pair(std::move(*message_bytes), std::move(signers));
  }

  // If sign_tx_param_ exists, decode encoded_serialized_msg from dApp to be
  // the serialized message byte array.
  std::vector<uint8_t> message_bytes;
  if (!Base58Decode(sign_tx_param_->encoded_serialized_msg, &message_bytes,
                    kSolanaMaxTxSize, false)) {
    return std::nullopt;
  }
  auto signers =
      SolanaMessage::GetSignerAccountsFromSerializedMessage(message_bytes);
  if (!signers || signers->empty()) {
    return std::nullopt;
  }

  return std::make_pair(std::move(message_bytes), std::move(*signers));
}

// Get serialized and signed transaction.
// A transaction contains a compact-array of signatures, followed by a message.
// A compact-array is serialized as the array length, followed by each array
// item. The array length is a special multi-byte encoding called compact-u16.
// See https://docs.solana.com/developing/programming-model/transactions.
std::optional<std::vector<uint8_t>>
SolanaTransaction::GetSignedTransactionBytes(
    KeyringService* keyring_service,
    const mojom::AccountIdPtr& selected_account,
    const std::vector<uint8_t>* selected_account_signature) const {
  if (!keyring_service && !selected_account_signature) {
    return std::nullopt;
  }

  if (selected_account_signature &&
      selected_account_signature->size() != kSolanaSignatureSize) {
    return std::nullopt;
  }

  auto message_signers_pair = GetSerializedMessage();
  if (!message_signers_pair) {
    return std::nullopt;
  }

  auto& message_bytes = message_signers_pair->first;
  auto& signers = message_signers_pair->second;

  // Preparing signatures.
  std::vector<uint8_t> transaction_bytes;
  if (signers.size() > UINT8_MAX) {
    return std::nullopt;
  }
  CompactU16Encode(signers.size(), &transaction_bytes);

  // Assign selected account's signature, and keep signatures for other signers
  // from dApp transaction if exists. Fill empty signatures for
  // non-selected-account signers if their signatures aren't passed by dApp
  // transaction. This would make sure solana-web3 JS transactions have entries
  // for all signers in the signatures property. We have to make sure of this
  // because our selected account's signature might be dropped later if
  // Transaction.signatures.length is not equal to number of required signers
  // for this transaction.
  uint8_t num_of_sig = 0;
  for (const auto& signer : signers) {
    if (base::EqualsCaseInsensitiveASCII(selected_account->address, signer)) {
      if (selected_account_signature) {
        transaction_bytes.insert(transaction_bytes.end(),
                                 selected_account_signature->begin(),
                                 selected_account_signature->end());
      } else {
        std::vector<uint8_t> signature =
            keyring_service->SignMessageBySolanaKeyring(selected_account,
                                                        message_bytes);
        transaction_bytes.insert(transaction_bytes.end(), signature.begin(),
                                 signature.end());
      }
      ++num_of_sig;
      continue;
    } else if (sign_tx_param_) {
      bool found = false;
      for (const auto& sig_pubkey_pair : sign_tx_param_->signatures) {
        if (sig_pubkey_pair->public_key == signer &&
            sig_pubkey_pair->signature &&
            sig_pubkey_pair->signature->size() == kSolanaSignatureSize) {
          transaction_bytes.insert(transaction_bytes.end(),
                                   sig_pubkey_pair->signature->begin(),
                                   sig_pubkey_pair->signature->end());
          ++num_of_sig;
          found = true;
          break;
        }
      }
      if (found) {
        continue;
      }
    }
    transaction_bytes.insert(transaction_bytes.end(), kSolanaSignatureSize, 0);
    ++num_of_sig;
  }
  DCHECK(num_of_sig == signers.size());

  // Message.
  transaction_bytes.insert(transaction_bytes.end(), message_bytes.begin(),
                           message_bytes.end());

  if (transaction_bytes.size() > kSolanaMaxTxSize) {
    return std::nullopt;
  }
  return transaction_bytes;
}

std::string SolanaTransaction::GetSignedTransaction(
    KeyringService* keyring_service,
    const mojom::AccountIdPtr& account_id) const {
  auto transaction_bytes =
      GetSignedTransactionBytes(keyring_service, account_id);
  if (!transaction_bytes) {
    return "";
  }
  return base::Base64Encode(*transaction_bytes);
}

std::string SolanaTransaction::GetUnsignedTransaction() const {
  auto message_signers_pair = GetSerializedMessage();
  if (!message_signers_pair) {
    return "";
  }

  auto& message_bytes = message_signers_pair->first;
  auto& signers = message_signers_pair->second;

  std::vector<uint8_t> transaction_bytes;

  CompactU16Encode(signers.size(), &transaction_bytes);

  // Insert an empty (default) signature for each signer.
  transaction_bytes.insert(transaction_bytes.end(),
                           kSolanaSignatureSize * signers.size(), 0);

  transaction_bytes.insert(transaction_bytes.end(), message_bytes.begin(),
                           message_bytes.end());

  if (transaction_bytes.size() > kSolanaMaxTxSize) {
    return "";
  }

  return base::Base64Encode(transaction_bytes);
}

std::string SolanaTransaction::GetBase64EncodedMessage() const {
  auto message_signers_pair = GetSerializedMessage();
  if (!message_signers_pair) {
    return "";
  }

  return base::Base64Encode(message_signers_pair->first);
}

mojom::SolanaTxDataPtr SolanaTransaction::ToSolanaTxData() const {
  auto solana_tx_data = message_.ToSolanaTxData();
  solana_tx_data->to_wallet_address = to_wallet_address_;
  solana_tx_data->token_address = token_address_;
  solana_tx_data->tx_type = tx_type_;
  solana_tx_data->lamports = lamports_;
  solana_tx_data->amount = amount_;
  solana_tx_data->fee_estimation = fee_estimation_.Clone();

  if (send_options_) {
    solana_tx_data->send_options = send_options_->ToMojomSendOptions();
  }

  if (sign_tx_param_) {
    solana_tx_data->sign_transaction_param = sign_tx_param_.Clone();
  }

  return solana_tx_data;
}

base::Value::Dict SolanaTransaction::ToValue() const {
  base::Value::Dict dict;
  dict.Set("message", message_.ToValue());
  dict.Set("to_wallet_address", to_wallet_address_);
  // We use the old key, spl_token_mint_address, for backwards compatibility
  // with when it didn't also represent compressed NFT identifiers.
  dict.Set("spl_token_mint_address", token_address_);
  dict.Set("tx_type", static_cast<int>(tx_type_));
  dict.Set("lamports", base::NumberToString(lamports_));
  dict.Set("amount", base::NumberToString(amount_));
  dict.Set("wired_tx", wired_tx_);

  if (send_options_) {
    dict.Set(kSendOptions, send_options_->ToValue());
  }

  if (sign_tx_param_) {
    base::Value::Dict sign_tx_param_dict;
    sign_tx_param_dict.Set(kEncodedSerializedMsg,
                           sign_tx_param_->encoded_serialized_msg);

    base::Value::List signatures_list;
    for (const auto& signature : sign_tx_param_->signatures) {
      base::Value::Dict signature_dict;
      signature_dict.Set(kPublicKey, signature->public_key);
      if (signature->signature) {
        signature_dict.Set(kSignature,
                           base::Base64Encode(*signature->signature));
      }
      signatures_list.Append(std::move(signature_dict));
    }
    sign_tx_param_dict.Set(kSignatures, std::move(signatures_list));
    dict.Set(kSignTxParam, std::move(sign_tx_param_dict));
  }

  if (fee_estimation_) {
    base::Value::Dict fee_estimation_dict;
    fee_estimation_dict.Set("base_fee",
                            base::NumberToString(fee_estimation_->base_fee));
    fee_estimation_dict.Set(
        "compute_units", base::NumberToString(fee_estimation_->compute_units));
    fee_estimation_dict.Set(
        "fee_per_compute_unit",
        base::NumberToString(fee_estimation_->fee_per_compute_unit));
    dict.Set("fee_estimation", std::move(fee_estimation_dict));
  }

  return dict;
}

void SolanaTransaction::set_tx_type(mojom::TransactionType tx_type) {
  DCHECK((tx_type >= mojom::TransactionType::Other &&
          tx_type <=
              mojom::TransactionType::
                  SolanaSPLTokenTransferWithAssociatedTokenAccountCreation) ||
         (tx_type >= mojom::TransactionType::SolanaDappSignAndSendTransaction &&
          tx_type <= mojom::TransactionType::SolanaSwap) ||
         tx_type == mojom::TransactionType::SolanaCompressedNftTransfer);
  tx_type_ = tx_type;
}

// static
std::unique_ptr<SolanaTransaction> SolanaTransaction::FromValue(
    const base::Value::Dict& value) {
  const base::Value::Dict* message_dict = value.FindDict("message");
  if (!message_dict) {
    return nullptr;
  }

  std::optional<SolanaMessage> message =
      SolanaMessage::FromValue(*message_dict);
  if (!message) {
    return nullptr;
  }

  auto tx = std::make_unique<SolanaTransaction>(std::move(*message));

  const auto* to_wallet_address = value.FindString("to_wallet_address");
  if (!to_wallet_address) {
    return nullptr;
  }
  tx->set_to_wallet_address(*to_wallet_address);

  // We use spl_token_mint_address as for backwards compatibility
  // with when it didn't also represent compressed NFT identifiers.
  const auto* token_address = value.FindString("spl_token_mint_address");
  if (!token_address) {
    return nullptr;
  }
  tx->set_token_address(*token_address);

  auto tx_type = value.FindInt("tx_type");
  if (!tx_type) {
    return nullptr;
  }
  tx->set_tx_type(static_cast<mojom::TransactionType>(*tx_type));

  const auto* lamports_string = value.FindString("lamports");
  uint64_t lamports = 0;
  if (!lamports_string || !base::StringToUint64(*lamports_string, &lamports)) {
    return nullptr;
  }
  tx->set_lamports(lamports);

  const auto* amount_string = value.FindString("amount");
  uint64_t amount = 0;
  if (!amount_string || !base::StringToUint64(*amount_string, &amount)) {
    return nullptr;
  }
  tx->set_amount(amount);

  const auto* wired_tx = value.FindString("wired_tx");
  if (wired_tx) {
    tx->set_wired_tx(*wired_tx);
  }

  const base::Value::Dict* send_options_value = value.FindDict(kSendOptions);
  if (send_options_value) {
    tx->set_send_options(SendOptions::FromValue(*send_options_value));
  }

  const base::Value::Dict* sign_tx_param_value = value.FindDict(kSignTxParam);
  if (sign_tx_param_value) {
    auto sign_tx_param = mojom::SolanaSignTransactionParam::New();
    const auto* encoded_serialized_msg =
        sign_tx_param_value->FindString(kEncodedSerializedMsg);
    if (!encoded_serialized_msg || encoded_serialized_msg->empty()) {
      return nullptr;
    }

    sign_tx_param->encoded_serialized_msg = *encoded_serialized_msg;
    const auto* signatures_value = sign_tx_param_value->FindList(kSignatures);
    if (!signatures_value) {
      return nullptr;
    }

    std::vector<mojom::SignaturePubkeyPairPtr> signatures;
    for (const auto& signature_value : *signatures_value) {
      const auto* signature_dict = signature_value.GetIfDict();
      if (!signature_dict) {
        return nullptr;
      }

      auto signature = mojom::SignaturePubkeyPair::New();
      const auto* public_key = signature_dict->FindString(kPublicKey);
      if (!public_key) {
        return nullptr;
      }
      signature->public_key = *public_key;

      const auto* signature_string = signature_dict->FindString(kSignature);
      if (signature_string) {
        signature->signature = base::Base64Decode(*signature_string);
      }

      signatures.push_back(std::move(signature));
    }
    sign_tx_param->signatures = std::move(signatures);
    tx->set_sign_tx_param(std::move(sign_tx_param));
  }

  const base::Value::Dict* fee_estimation_dict =
      value.FindDict("fee_estimation");
  if (fee_estimation_dict) {
    auto fee_estimation = mojom::SolanaFeeEstimation::New();
    const auto* base_fee_string = fee_estimation_dict->FindString("base_fee");
    uint64_t base_fee = 0;
    if (base_fee_string && base::StringToUint64(*base_fee_string, &base_fee)) {
      fee_estimation->base_fee = base_fee;
    }

    const auto* compute_units_string =
        fee_estimation_dict->FindString("compute_units");
    uint32_t compute_units = 0;
    if (compute_units_string &&
        base::StringToUint(*compute_units_string, &compute_units)) {
      fee_estimation->compute_units = compute_units;
    }

    const auto* fee_per_compute_unit_string =
        fee_estimation_dict->FindString("fee_per_compute_unit");
    uint64_t fee_per_compute_unit = 0;
    if (fee_per_compute_unit_string &&
        base::StringToUint64(*fee_per_compute_unit_string,
                             &fee_per_compute_unit)) {
      fee_estimation->fee_per_compute_unit = fee_per_compute_unit;
    }

    if (fee_estimation->base_fee != 0 || fee_estimation->compute_units != 0 ||
        fee_estimation->fee_per_compute_unit != 0) {
      tx->set_fee_estimation(std::move(fee_estimation));
    }
  }

  return tx;
}

// static
std::unique_ptr<SolanaTransaction> SolanaTransaction::FromSolanaTxData(
    mojom::SolanaTxDataPtr solana_tx_data) {
  std::vector<SolanaInstruction> instructions;
  SolanaInstruction::FromMojomSolanaInstructions(solana_tx_data->instructions,
                                                 &instructions);
  std::vector<SolanaAddress> static_account_keys;
  for (const auto& base58_account : solana_tx_data->static_account_keys) {
    auto addr = SolanaAddress::FromBase58(base58_account);
    if (!addr) {
      return nullptr;
    }
    static_account_keys.emplace_back(*addr);
  }

  auto addr_table_lookups = SolanaMessageAddressTableLookup::FromMojomArray(
      solana_tx_data->address_table_lookups);
  if (!addr_table_lookups) {
    return nullptr;
  }

  auto tx = std::make_unique<SolanaTransaction>(
      solana_tx_data->version, solana_tx_data->recent_blockhash,
      solana_tx_data->last_valid_block_height, solana_tx_data->fee_payer,
      SolanaMessageHeader::FromMojom(solana_tx_data->message_header),
      std::move(static_account_keys), std::move(instructions),
      std::move(*addr_table_lookups));
  tx->set_to_wallet_address(solana_tx_data->to_wallet_address);
  tx->set_token_address(solana_tx_data->token_address);
  tx->set_tx_type(solana_tx_data->tx_type);
  tx->set_lamports(solana_tx_data->lamports);
  tx->set_amount(solana_tx_data->amount);
  tx->set_send_options(SendOptions::FromMojomSendOptions(
      std::move(solana_tx_data->send_options)));
  tx->set_sign_tx_param(std::move(solana_tx_data->sign_transaction_param));

  return tx;
}

// static
std::unique_ptr<SolanaTransaction>
SolanaTransaction::FromSignedTransactionBytes(
    const std::vector<uint8_t>& bytes) {
  if (bytes.empty() || bytes.size() > kSolanaMaxTxSize) {
    return nullptr;
  }

  size_t index = 0;
  auto ret = CompactU16Decode(bytes, index);
  if (!ret) {
    return nullptr;
  }
  const uint16_t num_of_signatures = std::get<0>(*ret);
  index += std::get<1>(*ret);
  if (index + num_of_signatures * kSolanaSignatureSize > bytes.size()) {
    return nullptr;
  }
  std::vector<uint8_t> signatures(
      bytes.begin() + index,
      bytes.begin() + index + num_of_signatures * kSolanaSignatureSize);
  index += num_of_signatures * kSolanaSignatureSize;
  auto message = SolanaMessage::Deserialize(
      std::vector<uint8_t>(bytes.begin() + index, bytes.end()));
  if (!message) {
    return nullptr;
  }

  return std::make_unique<SolanaTransaction>(std::move(*message),
                                             std::move(signatures));
}

bool SolanaTransaction::IsPartialSigned() const {
  if (!sign_tx_param_) {
    return false;
  }

  for (const auto& sig_pubkey_pair : sign_tx_param_->signatures) {
    // Has non-empty signature.
    if (sig_pubkey_pair->signature && !sig_pubkey_pair->signature->empty() &&
        sig_pubkey_pair->signature !=
            std::vector<uint8_t>(kSolanaSignatureSize, 0)) {
      return true;
    }
  }

  return false;
}

}  // namespace brave_wallet
