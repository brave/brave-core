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

namespace solana {

namespace system_program {

std::optional<SolanaInstruction> Transfer(const std::string& from_pubkey,
                                          const std::string& to_pubkey,
                                          uint64_t lamport);

}  // namespace system_program

namespace spl_token_program {

std::optional<SolanaInstruction> Transfer(
    const std::string& token_program_id,
    const std::string& source_pubkey,
    const std::string& destination_pubkey,
    const std::string& authority_pubkey,
    const std::vector<std::string>& signer_pubkeys,
    uint64_t amount);

}  // namespace spl_token_program

namespace spl_associated_token_account_program {

std::optional<SolanaInstruction> CreateAssociatedTokenAccount(
    const std::string& funding_address,
    const std::string& wallet_address,
    const std::string& associated_token_account_address,
    const std::string& spl_token_mint_address);

}  // namespace spl_associated_token_account_program

namespace compute_budget_program {

SolanaInstruction SetComputeUnitLimit(uint32_t units);

SolanaInstruction SetComputeUnitPrice(uint64_t price);

}  // namespace compute_budget_program

}  // namespace solana

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_BUILDER_H_
