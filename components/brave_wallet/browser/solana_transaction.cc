/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#include "brave/components/brave_wallet/browser/solana_transaction.h"

#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace brave_wallet {

namespace {

constexpr char kSendOptions[] = "send_options";

// Below are using camel cases so we can handle the parameters from dApp
// requests directly with the same key.
constexpr char kMaxRetries[] = "maxRetries";
constexpr char kPreflightCommitment[] = "preflightCommitment";
constexpr char kSkipPreflight[] = "skipPreflight";

bool IsValidCommitmentString(const std::string& commitment) {
  return commitment == "processed" || commitment == "confirmed" ||
         commitment == "finalized";
}

}  // namespace

SolanaTransaction::SendOptions::SendOptions() = default;
SolanaTransaction::SendOptions::~SendOptions() = default;
SolanaTransaction::SendOptions::SendOptions(
    const SolanaTransaction::SendOptions&) = default;
SolanaTransaction::SendOptions::SendOptions(
    absl::optional<uint64_t> max_retries_param,
    absl::optional<std::string> preflight_commitment_param,
    absl::optional<bool> skip_preflight_param)
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
absl::optional<SolanaTransaction::SendOptions>
SolanaTransaction::SendOptions::FromValue(absl::optional<base::Value> value) {
  if (!value)
    return absl::nullopt;
  return FromValue(*value);
}

// static
absl::optional<SolanaTransaction::SendOptions>
SolanaTransaction::SendOptions::FromValue(const base::Value& value) {
  const base::Value::Dict* options_dict = value.GetIfDict();
  if (!options_dict)
    return absl::nullopt;

  SolanaTransaction::SendOptions options;

  if (auto* max_retries_string = options_dict->FindString(kMaxRetries)) {
    // Type of maxRetries is string when it's from preference values.
    uint64_t max_retries = 0;
    if (base::StringToUint64(*max_retries_string, &max_retries)) {
      options.max_retries = max_retries;
    }
  } else if (auto max_retries_number = options_dict->FindDouble(kMaxRetries)) {
    // Type of maxRetries is number when it's from dApp requests.
    // We cap the maximum to 2^53-1 here for double precision, it's safe here
    // because it does not make sense for dApps to set maxRetries that large.
    if (*max_retries_number >= 0 &&
        *max_retries_number <= static_cast<double>(kMaxSafeIntegerUint64)) {
      options.max_retries = max_retries_number;
    }
  }

  auto* commitment = options_dict->FindString(kPreflightCommitment);
  if (commitment && IsValidCommitmentString(*commitment)) {
    options.preflight_commitment = *commitment;
  }
  options.skip_preflight = options_dict->FindBool(kSkipPreflight);

  return options;
}

base::Value SolanaTransaction::SendOptions::ToValue() const {
  base::Value options(base::Value::Type::DICTIONARY);
  if (max_retries)
    options.SetStringKey(kMaxRetries, base::NumberToString(*max_retries));
  if (preflight_commitment)
    options.SetStringKey(kPreflightCommitment, *preflight_commitment);
  if (skip_preflight)
    options.SetBoolKey(kSkipPreflight, *skip_preflight);
  return options;
}

// static
absl::optional<SolanaTransaction::SendOptions>
SolanaTransaction::SendOptions::FromMojomSendOptions(
    mojom::SolanaSendTransactionOptionsPtr mojom_options) {
  if (!mojom_options)
    return absl::nullopt;

  SendOptions options;
  if (mojom_options->max_retries)
    options.max_retries = mojom_options->max_retries->max_retries;
  if (mojom_options->skip_preflight)
    options.skip_preflight = mojom_options->skip_preflight->skip_preflight;
  if (mojom_options->preflight_commitment &&
      IsValidCommitmentString(*mojom_options->preflight_commitment)) {
    options.preflight_commitment = mojom_options->preflight_commitment;
  }
  return options;
}

mojom::SolanaSendTransactionOptionsPtr
SolanaTransaction::SendOptions::ToMojomSendOptions() const {
  auto send_options = mojom::SolanaSendTransactionOptions::New();
  if (max_retries)
    send_options->max_retries = mojom::OptionalMaxRetries::New(*max_retries);
  if (preflight_commitment)
    send_options->preflight_commitment = *preflight_commitment;
  if (skip_preflight)
    send_options->skip_preflight =
        mojom::OptionalSkipPreflight::New(*skip_preflight);
  return send_options;
}

SolanaTransaction::SolanaTransaction(
    const std::string& recent_blockhash,
    uint64_t last_valid_block_height,
    const std::string& fee_payer,
    std::vector<SolanaInstruction>&& instructions)
    : message_(recent_blockhash,
               last_valid_block_height,
               fee_payer,
               std::move(instructions)) {}

SolanaTransaction::SolanaTransaction(SolanaMessage&& message)
    : message_(std::move(message)) {}

SolanaTransaction::SolanaTransaction(SolanaMessage&& message,
                                     const std::vector<uint8_t>& signatures)
    : message_(std::move(message)), signatures_(signatures) {}

SolanaTransaction::SolanaTransaction(const SolanaTransaction&) = default;
SolanaTransaction::~SolanaTransaction() = default;

bool SolanaTransaction::operator==(const SolanaTransaction& tx) const {
  return message_ == tx.message_ && signatures_ == tx.signatures_ &&
         to_wallet_address_ == tx.to_wallet_address_ &&
         spl_token_mint_address_ == tx.spl_token_mint_address_ &&
         tx_type_ == tx.tx_type_ && lamports_ == tx.lamports_ &&
         amount_ == tx.amount_ && send_options_ == tx.send_options_;
}

bool SolanaTransaction::operator!=(const SolanaTransaction& tx) const {
  return !operator==(tx);
}

// Get serialized and signed transaction.
// A transaction contains a compact-array of signatures, followed by a message.
// A compact-array is serialized as the array length, followed by each array
// item. The array length is a special multi-byte encoding called compact-u16.
// See https://docs.solana.com/developing/programming-model/transactions.
absl::optional<std::vector<uint8_t>>
SolanaTransaction::GetSignedTransactionBytes(
    KeyringService* keyring_service) const {
  if (!keyring_service)
    return absl::nullopt;

  std::vector<std::string> signers;
  auto message_bytes = message_.Serialize(&signers);
  if (!message_bytes || signers.empty())
    return absl::nullopt;

  std::vector<uint8_t> transaction_bytes;
  // Compact array of signatures.
  CompactU16Encode(signers.size(), &transaction_bytes);
  for (const auto& signer : signers) {
    std::vector<uint8_t> signature = keyring_service->SignMessage(
        mojom::kSolanaKeyringId, signer, message_bytes.value());
    transaction_bytes.insert(transaction_bytes.end(), signature.begin(),
                             signature.end());
  }

  // Message.
  transaction_bytes.insert(transaction_bytes.end(), message_bytes->begin(),
                           message_bytes->end());

  if (transaction_bytes.size() > kSolanaMaxTxSize)
    return absl::nullopt;
  return transaction_bytes;
}

std::string SolanaTransaction::GetSignedTransaction(
    KeyringService* keyring_service) const {
  auto transaction_bytes = GetSignedTransactionBytes(keyring_service);
  if (!transaction_bytes)
    return "";
  return base::Base64Encode(*transaction_bytes);
}

std::string SolanaTransaction::GetBase64EncodedMessage() const {
  auto message_bytes = message_.Serialize(nullptr /* signers */);
  if (!message_bytes)
    return "";

  return base::Base64Encode(*message_bytes);
}

mojom::SolanaTxDataPtr SolanaTransaction::ToSolanaTxData() const {
  auto solana_tx_data = message_.ToSolanaTxData();
  solana_tx_data->to_wallet_address = to_wallet_address_;
  solana_tx_data->spl_token_mint_address = spl_token_mint_address_;
  solana_tx_data->tx_type = tx_type_;
  solana_tx_data->lamports = lamports_;
  solana_tx_data->amount = amount_;

  if (send_options_) {
    solana_tx_data->send_options = send_options_->ToMojomSendOptions();
  }

  return solana_tx_data;
}

base::Value SolanaTransaction::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetKey("message", message_.ToValue());

  dict.SetStringKey("to_wallet_address", to_wallet_address_);
  dict.SetStringKey("spl_token_mint_address", spl_token_mint_address_);
  dict.SetIntKey("tx_type", static_cast<int>(tx_type_));
  dict.SetStringKey("lamports", base::NumberToString(lamports_));
  dict.SetStringKey("amount", base::NumberToString(amount_));
  if (send_options_)
    dict.SetKey(kSendOptions, send_options_->ToValue());

  return dict;
}

void SolanaTransaction::set_tx_type(mojom::TransactionType tx_type) {
  DCHECK((tx_type >= mojom::TransactionType::Other &&
          tx_type <=
              mojom::TransactionType::
                  SolanaSPLTokenTransferWithAssociatedTokenAccountCreation) ||
         tx_type == mojom::TransactionType::SolanaDappSignAndSendTransaction ||
         tx_type == mojom::TransactionType::SolanaDappSignTransaction);
  tx_type_ = tx_type;
}

// static
absl::optional<SolanaTransaction> SolanaTransaction::FromValue(
    const base::Value& value) {
  if (!value.is_dict())
    return absl::nullopt;
  const base::Value* message_dict = value.FindKey("message");
  if (!message_dict || !message_dict->is_dict())
    return absl::nullopt;

  absl::optional<SolanaMessage> message =
      SolanaMessage::FromValue(*message_dict);
  if (!message)
    return absl::nullopt;

  auto tx = SolanaTransaction(std::move(*message));

  const auto* to_wallet_address = value.FindStringKey("to_wallet_address");
  if (!to_wallet_address)
    return absl::nullopt;
  tx.set_to_wallet_address(*to_wallet_address);

  const auto* spl_token_mint_address =
      value.FindStringKey("spl_token_mint_address");
  if (!spl_token_mint_address)
    return absl::nullopt;
  tx.set_spl_token_mint_address(*spl_token_mint_address);

  auto tx_type = value.FindIntKey("tx_type");
  if (!tx_type)
    return absl::nullopt;
  tx.set_tx_type(static_cast<mojom::TransactionType>(*tx_type));

  const auto* lamports_string = value.FindStringKey("lamports");
  uint64_t lamports = 0;
  if (!lamports_string || !base::StringToUint64(*lamports_string, &lamports))
    return absl::nullopt;
  tx.set_lamports(lamports);

  const auto* amount_string = value.FindStringKey("amount");
  uint64_t amount = 0;
  if (!amount_string || !base::StringToUint64(*amount_string, &amount))
    return absl::nullopt;
  tx.set_amount(amount);
  const base::Value* send_options_value = value.FindDictKey(kSendOptions);
  if (send_options_value)
    tx.set_send_options(SendOptions::FromValue(*send_options_value));

  return tx;
}

// static
std::unique_ptr<SolanaTransaction> SolanaTransaction::FromSolanaTxData(
    mojom::SolanaTxDataPtr solana_tx_data) {
  std::vector<SolanaInstruction> instructions;
  SolanaInstruction::FromMojomSolanaInstructions(solana_tx_data->instructions,
                                                 &instructions);
  auto tx = std::make_unique<SolanaTransaction>(
      solana_tx_data->recent_blockhash, solana_tx_data->last_valid_block_height,
      solana_tx_data->fee_payer, std::move(instructions));
  tx->set_to_wallet_address(solana_tx_data->to_wallet_address);
  tx->set_spl_token_mint_address(solana_tx_data->spl_token_mint_address);
  tx->set_tx_type(solana_tx_data->tx_type);
  tx->set_lamports(solana_tx_data->lamports);
  tx->set_amount(solana_tx_data->amount);
  tx->set_send_options(SendOptions::FromMojomSendOptions(
      std::move(solana_tx_data->send_options)));

  return tx;
}

// static
absl::optional<SolanaTransaction> SolanaTransaction::FromSignedTransactionBytes(
    const std::vector<uint8_t>& bytes) {
  if (bytes.empty() || bytes.size() > kSolanaMaxTxSize)
    return absl::nullopt;

  size_t index = 0;
  auto ret = CompactU16Decode(bytes, index);
  if (!ret)
    return absl::nullopt;
  const uint16_t num_of_signatures = std::get<0>(*ret);
  index += std::get<1>(*ret);
  if (index + num_of_signatures * kSolanaSignatureSize > bytes.size())
    return absl::nullopt;
  const std::vector<uint8_t> signatures(
      bytes.begin() + index,
      bytes.begin() + index + num_of_signatures * kSolanaSignatureSize);
  index += num_of_signatures * kSolanaSignatureSize;
  auto message = SolanaMessage::Deserialize(
      std::vector<uint8_t>(bytes.begin() + index, bytes.end()));
  if (!message)
    return absl::nullopt;

  return SolanaTransaction(std::move(message->first), signatures);
}

}  // namespace brave_wallet
