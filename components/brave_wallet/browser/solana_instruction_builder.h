/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_BUILDER_H_

#include <optional>
#include <string>
#include <vector>

namespace brave_wallet {

class SolanaInstruction;
struct SolCompressedNftProofData;

namespace solana {

namespace system_program {

std::optional<SolanaInstruction> Transfer(const std::string& from_pubkey,
                                          const std::string& to_pubkey,
                                          uint64_t lamport);

}  // namespace system_program

namespace spl_token_program {

// https://github.com/solana-labs/solana-program-library/blob/d1c03cc6164ebdeebef19190de28b7a71f4e4cb6/token/program-2022/src/instruction.rs#L336
std::optional<SolanaInstruction> TransferChecked(
    const std::string& token_program_id,
    const std::string& source_pubkey,
    const std::string& mint_address,
    const std::string& destination_pubkey,
    const std::string& authority_pubkey,
    const std::vector<std::string>& signer_pubkeys,
    uint64_t amount,
    uint8_t decimals);

}  // namespace spl_token_program

namespace spl_associated_token_account_program {

std::optional<SolanaInstruction> CreateAssociatedTokenAccount(
    const std::string& token_program_id,
    const std::string& funding_address,
    const std::string& wallet_address,
    const std::string& associated_token_account_address,
    const std::string& spl_token_mint_address);

}  // namespace spl_associated_token_account_program

namespace compute_budget_program {

SolanaInstruction SetComputeUnitLimit(uint32_t units);

SolanaInstruction SetComputeUnitPrice(uint64_t price);

}  // namespace compute_budget_program

namespace bubblegum_program {

const std::vector<uint8_t> kTransferInstructionDiscriminator = {
    163, 52, 200, 231, 140, 3, 69, 186};

// https://github.com/metaplex-foundation/mpl-bubblegum/blob/5b3cdfc6b236773be70dc1f0b0cb84badf881248/clients/js-solita/src/generated/instructions/transfer.ts#L81
std::optional<SolanaInstruction> Transfer(
    uint32_t canopy_depth,
    const std::string& tree_authority,
    const std::string& new_leaf_owner,
    const SolCompressedNftProofData& proof);

}  // namespace bubblegum_program

}  // namespace solana

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_BUILDER_H_
