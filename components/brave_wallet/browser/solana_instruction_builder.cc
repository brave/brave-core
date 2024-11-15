/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"

#include <optional>
#include <utility>

#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_wallet/browser/simple_hash_client.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"

namespace brave_wallet::solana {

using mojom::SolanaComputeBudgetInstruction::kSetComputeUnitLimit;
using mojom::SolanaComputeBudgetInstruction::kSetComputeUnitPrice;
using mojom::SolanaSystemInstruction::kTransfer;
using mojom::SolanaTokenInstruction::kTransferChecked;

namespace system_program {

// Transfer lamports from funding account (from) to recipient account (to).
// Account references:
//   0. Funding account [signer, writable].
//   1. Recipient account [non-signer, writable].
// Instruction data: u32 instruction index and u64 lamport.
std::optional<SolanaInstruction> Transfer(const std::string& from_pubkey,
                                          const std::string& to_pubkey,
                                          uint64_t lamport) {
  if (from_pubkey.empty() || to_pubkey.empty()) {
    return std::nullopt;
  }

  // Instruction data is consisted of u32 instruction index and u64 lamport.
  std::array<uint8_t, 12> instruction_data;
  auto span_writer = base::SpanWriter(base::span(instruction_data));
  span_writer.WriteU32LittleEndian(base::checked_cast<uint32_t>(kTransfer));
  span_writer.WriteU64LittleEndian(lamport);
  CHECK(span_writer.remaining_span().empty());

  return SolanaInstruction(
      mojom::kSolanaSystemProgramId,
      std::vector<SolanaAccountMeta>(
          {SolanaAccountMeta(from_pubkey, std::nullopt, true, true),
           SolanaAccountMeta(to_pubkey, std::nullopt, false, true)}),
      instruction_data);
}

}  // namespace system_program

namespace spl_token_program {

// Transfers amount of tokens from source account to destination either
// directly or via a delegate.
// Account references for single owner/delegate:
//   0. Source account [non-signer, writable].
//   1. The token mint [non-signer, readonly].
//   2. Destination account [non-signer, writable].
//   3. Authority account (source account's owner/delegate) [signer, readonly]
// Account references for multisignature owner/delegate:
//   0. Source account [non-signer, writable].
//   1. The token mint [non-signer, readonly].
//   2. Destination account [non-signer, writable].
//   3. Authority account (source account's multisignature owner/delegate)
//      [non-signer, readonly]
//   4~4+M. M signer accounts [signer, readonly].
// Instruction data: u8 instruction index and u64 amount.
std::optional<SolanaInstruction> TransferChecked(
    const std::string& token_program_id,
    const std::string& source_pubkey,
    const std::string& mint_address,
    const std::string& destination_pubkey,
    const std::string& authority_pubkey,
    const std::vector<std::string>& signer_pubkeys,
    uint64_t amount,
    uint8_t decimals) {
  if (token_program_id.empty() || source_pubkey.empty() ||
      mint_address.empty() || destination_pubkey.empty() ||
      authority_pubkey.empty()) {
    return std::nullopt;
  }

  // Instruction data is consisted of u8 instruction index and u64 amount.
  std::array<uint8_t, 10> instruction_data;
  auto span_writer = base::SpanWriter(base::span(instruction_data));
  span_writer.WriteU8LittleEndian(
      base::checked_cast<uint8_t>(kTransferChecked));
  span_writer.WriteU64LittleEndian(amount);
  span_writer.WriteU8LittleEndian(decimals);
  CHECK(span_writer.remaining_span().empty());

  std::vector<SolanaAccountMeta> account_metas = {
      SolanaAccountMeta(source_pubkey, std::nullopt, false, true),
      SolanaAccountMeta(mint_address, std::nullopt, false, false),
      SolanaAccountMeta(destination_pubkey, std::nullopt, false, true),
      SolanaAccountMeta(authority_pubkey, std::nullopt, signer_pubkeys.empty(),
                        false)};

  for (const auto& signer_pubkey : signer_pubkeys) {
    account_metas.emplace_back(signer_pubkey, std::nullopt, true, false);
  }

  return SolanaInstruction(token_program_id, std::move(account_metas),
                           instruction_data);
}

}  // namespace spl_token_program

namespace spl_associated_token_account_program {

// Create an associated token account for the given wallet address and token
// mint.
// Account references:
// 0. Funding account (must be a system account) [signer, writeable].
// 1. Associated token account address to be created [non-signer, writable].
// 2. Wallet address for the new associated token account [non-signer,
//    readonly].
// 3. The token mint for the new associated token account [non-signer,
//    readonly].
// 4. System program [non-signer, readonly].
// 5. SPL Token program [non-signer, readonly].
// Ref:
// https://docs.rs/spl-associated-token-account/1.1.2/spl_associated_token_account/instruction/enum.AssociatedTokenAccountInstruction.html#variant.Create
std::optional<SolanaInstruction> CreateAssociatedTokenAccount(
    const std::string& token_program_id,
    const std::string& funding_address,
    const std::string& wallet_address,
    const std::string& associated_token_account_address,
    const std::string& spl_token_mint_address) {
  if (token_program_id.empty() || funding_address.empty() ||
      wallet_address.empty() || associated_token_account_address.empty() ||
      spl_token_mint_address.empty()) {
    return std::nullopt;
  }

  std::vector<SolanaAccountMeta> account_metas = {
      SolanaAccountMeta(funding_address, std::nullopt, true, true),
      SolanaAccountMeta(associated_token_account_address, std::nullopt, false,
                        true),
      SolanaAccountMeta(wallet_address, std::nullopt, false, false),
      SolanaAccountMeta(spl_token_mint_address, std::nullopt, false, false),
      SolanaAccountMeta(mojom::kSolanaSystemProgramId, std::nullopt, false,
                        false),
      SolanaAccountMeta(token_program_id, std::nullopt, false, false)};
  return SolanaInstruction(mojom::kSolanaAssociatedTokenProgramId,
                           std::move(account_metas), std::vector<uint8_t>());
}

}  // namespace spl_associated_token_account_program

namespace compute_budget_program {

// Set the compute unit limit for transaction execution.
// https://docs.rs/solana-sdk/1.18.14/src/solana_sdk/compute_budget.rs.html#33
SolanaInstruction SetComputeUnitLimit(uint32_t units) {
  std::array<uint8_t, 5> instruction_data;
  auto span_writer = base::SpanWriter(base::span(instruction_data));
  span_writer.WriteU8LittleEndian(
      base::checked_cast<uint8_t>(kSetComputeUnitLimit));
  span_writer.WriteU32LittleEndian(units);
  CHECK(span_writer.remaining_span().empty());

  return SolanaInstruction(mojom::kSolanaComputeBudgetProgramId, {},
                           instruction_data);
}

// Set the compute unit price for transaction execution.
// https://docs.rs/solana-sdk/1.18.14/src/solana_sdk/compute_budget.rs.html#36
SolanaInstruction SetComputeUnitPrice(uint64_t price) {
  std::array<uint8_t, 9> instruction_data;
  auto span_writer = base::SpanWriter(base::span(instruction_data));
  span_writer.WriteU8LittleEndian(
      base::checked_cast<uint8_t>(kSetComputeUnitPrice));
  span_writer.WriteU64LittleEndian(price);
  CHECK(span_writer.remaining_span().empty());

  return SolanaInstruction(mojom::kSolanaComputeBudgetProgramId, {},
                           instruction_data);
}

}  // namespace compute_budget_program

namespace bubblegum_program {

std::optional<SolanaInstruction> Transfer(
    uint32_t canopy_depth,
    const std::string& tree_authority,
    const std::string& new_leaf_owner,
    const SolCompressedNftProofData& proof) {
  const std::string log_wrapper = "noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV";

  // Init instruction data with the instruction discriminator.
  std::array<uint8_t, 116> instruction_data;
  auto span_writer = base::SpanWriter(base::span(instruction_data));

  span_writer.Write(kTransferInstructionDiscriminator);

  std::vector<uint8_t> root_bytes;
  if (!Base58Decode(proof.root, &root_bytes, kSolanaHashSize)) {
    return std::nullopt;
  }
  span_writer.Write(root_bytes);

  std::vector<uint8_t> data_hash_bytes;
  if (!Base58Decode(proof.data_hash, &data_hash_bytes, kSolanaHashSize)) {
    return std::nullopt;
  }
  span_writer.Write(data_hash_bytes);

  std::vector<uint8_t> creator_hash_bytes;
  if (!Base58Decode(proof.creator_hash, &creator_hash_bytes, kSolanaHashSize)) {
    return std::nullopt;
  }
  span_writer.Write(creator_hash_bytes);

  // Nonce
  // Use leaf.index for nonce like the example
  // https://solana.com/developers/guides/javascript/compressed-nfts#build-the-transfer-instruction
  span_writer.WriteU64LittleEndian(proof.leaf_index);

  // Index
  span_writer.WriteU32LittleEndian(proof.leaf_index);

  CHECK(span_writer.remaining_span().empty());

  // Create account metas.
  std::vector<SolanaAccountMeta> account_metas({
      SolanaAccountMeta(tree_authority, std::nullopt, false, false),
      SolanaAccountMeta(proof.owner, std::nullopt, false, false),
      SolanaAccountMeta(proof.owner, std::nullopt, false, false),
      SolanaAccountMeta(new_leaf_owner, std::nullopt, false, false),
      SolanaAccountMeta(proof.merkle_tree, std::nullopt, false, true),
      SolanaAccountMeta(log_wrapper, std::nullopt, false, false),
      SolanaAccountMeta(mojom::kSolanaAccountCompressionProgramId, std::nullopt,
                        false, false),
      SolanaAccountMeta(mojom::kSolanaSystemProgramId, std::nullopt, false,
                        false),
  });

  // Add on the slice of our proof
  if (proof.proof.size() < canopy_depth) {
    return std::nullopt;
  }
  for (auto& proof_item :
       base::span(proof.proof).first(proof.proof.size() - proof.canopy_depth)) {
    account_metas.emplace_back(proof_item, std::nullopt, false, false);
  }

  return SolanaInstruction(mojom::kSolanaBubbleGumProgramId,
                           std::move(account_metas), instruction_data);
}

}  // namespace bubblegum_program

}  // namespace brave_wallet::solana
