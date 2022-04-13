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
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace brave_wallet {

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

SolanaTransaction::SolanaTransaction(const SolanaTransaction&) = default;
SolanaTransaction::~SolanaTransaction() = default;

bool SolanaTransaction::operator==(const SolanaTransaction& tx) const {
  return message_ == tx.message_ &&
         to_wallet_address_ == tx.to_wallet_address_ &&
         spl_token_mint_address_ == tx.spl_token_mint_address_ &&
         tx_type_ == tx.tx_type_ && lamports_ == tx.lamports_ &&
         amount_ == tx.amount_;
}

// Get serialized and signed transaction.
// A transaction contains a compact-array of signatures, followed by a message.
// A compact-array is serialized as the array length, followed by each array
// item. The array length is a special multi-byte encoding called compact-u16.
// See https://docs.solana.com/developing/programming-model/transactions.
std::string SolanaTransaction::GetSignedTransaction(
    KeyringService* keyring_service) const {
  if (!keyring_service)
    return "";

  std::vector<std::string> signers;
  auto message_bytes = message_.Serialize(&signers);
  if (!message_bytes || signers.empty())
    return "";

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
    return "";

  return base::Base64Encode(transaction_bytes);
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

  return dict;
}

void SolanaTransaction::set_tx_type(mojom::TransactionType tx_type) {
  DCHECK(tx_type >= mojom::TransactionType::Other &&
         tx_type <=
             mojom::TransactionType::
                 SolanaSPLTokenTransferWithAssociatedTokenAccountCreation);
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
  return tx;
}

}  // namespace brave_wallet
