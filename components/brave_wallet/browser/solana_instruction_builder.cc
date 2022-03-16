/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"

#include <type_traits>
#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "build/build_config.h"

namespace {

// Solana uses bincode::serialize when encoding instruction data, which encodes
// unsigned numbers in little endian byte order.
template <typename T>
void UintToLEBytes(T val, std::vector<uint8_t>* bytes) {
  static_assert(
      std::is_same<T, uint64_t>::value || std::is_same<T, uint32_t>::value,
      "Incorrect type passed to function UintToLEBytes.");

  DCHECK(bytes);
  size_t vec_size = sizeof(T) / sizeof(uint8_t);
  *bytes = std::vector<uint8_t>(vec_size);

  uint8_t* ptr = reinterpret_cast<uint8_t*>(&val);
  for (size_t i = 0; i < vec_size; i++) {
#if defined(ARCH_CPU_LITTLE_ENDIAN)
    bytes->at(i) = *ptr++;
#else
    bytes->at(vec_size - 1 - i) = *ptr++;
#endif
  }
}

}  // namespace

namespace brave_wallet {

namespace solana {

namespace system_program {

// Transfer lamports from funding account (from) to recipient account (to).
// Account references:
//   0. Funding account [signer, writable].
//   1. Recipient account [non-signer, writable].
// Insturction data: u32 instruction index and u64 lamport.
absl::optional<SolanaInstruction> Transfer(const std::string& from_pubkey,
                                           const std::string& to_pubkey,
                                           uint64_t lamport) {
  if (from_pubkey.empty() || to_pubkey.empty())
    return absl::nullopt;

  // Instruction data is consisted of u32 instruction index and u64 lamport.
  std::vector<uint8_t> instruction_data;
  UintToLEBytes(static_cast<uint32_t>(SolanaSystemInstruction::kTransfer),
                &instruction_data);

  std::vector<uint8_t> lamport_bytes;
  UintToLEBytes(lamport, &lamport_bytes);
  instruction_data.insert(instruction_data.end(), lamport_bytes.begin(),
                          lamport_bytes.end());

  return SolanaInstruction(kSolanaSystemProgramId,
                           std::vector<SolanaAccountMeta>(
                               {SolanaAccountMeta(from_pubkey, true, true),
                                SolanaAccountMeta(to_pubkey, false, true)}),
                           instruction_data);
}

}  // namespace system_program

namespace spl_token_program {

// Transfers amount of tokens from source account to destination either
// directly or via a delegate.
// Account references for single owner/delegate:
//   0. Source account [non-signer, writable].
//   1. Destination account [non-signer, writable].
//   2. Authority account (source account's owner/delegate) [signer, readonly]
// Account references for multisignature owner/delegate:
//   0. Source account [non-signer, writable].
//   1. Destination account [non-signer, writable].
//   2. Authority account (source account's multisignature owner/delegate)
//      [non-signer, readonly]
//   3~3+M. M signer accounts [signer, readonly].
// Insturction data: u8 instruction index and u64 amount.
absl::optional<SolanaInstruction> Transfer(
    const std::string& token_program_id,
    const std::string& source_pubkey,
    const std::string& destination_pubkey,
    const std::string& authority_pubkey,
    const std::vector<std::string>& signer_pubkeys,
    uint64_t amount) {
  if (token_program_id.empty() || source_pubkey.empty() ||
      destination_pubkey.empty() || authority_pubkey.empty())
    return absl::nullopt;

  // Instruction data is consisted of u8 instruction index and u64 amount.
  std::vector<uint8_t> instruction_data = {
      static_cast<uint8_t>(SolanaTokenInstruction::kTransfer)};

  std::vector<uint8_t> amount_bytes;
  UintToLEBytes(amount, &amount_bytes);
  instruction_data.insert(instruction_data.end(), amount_bytes.begin(),
                          amount_bytes.end());

  std::vector<SolanaAccountMeta> account_metas = {
      SolanaAccountMeta(source_pubkey, false, true),
      SolanaAccountMeta(destination_pubkey, false, true),
      SolanaAccountMeta(authority_pubkey, signer_pubkeys.empty(), false)};

  for (const auto& signer_pubkey : signer_pubkeys)
    account_metas.push_back(SolanaAccountMeta(signer_pubkey, true, false));

  return SolanaInstruction(token_program_id, std::move(account_metas),
                           instruction_data);
}

}  // namespace spl_token_program

}  // namespace solana

}  // namespace brave_wallet
