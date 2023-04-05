/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_COMPILED_INSTRUCTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_COMPILED_INSTRUCTION_H_

#include <string>
#include <vector>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class SolanaAddress;
class SolanaInstruction;
class SolanaMessageAddressTableLookup;

// https://docs.rs/solana-sdk/1.14.12/solana_sdk/instruction/struct.CompiledInstruction.html
class SolanaCompiledInstruction {
 public:
  SolanaCompiledInstruction(uint8_t program_id_index,
                            const std::vector<uint8_t>& account_indexes,
                            const std::vector<uint8_t>& data);
  SolanaCompiledInstruction(const SolanaCompiledInstruction&) = delete;
  SolanaCompiledInstruction(SolanaCompiledInstruction&&);
  SolanaCompiledInstruction& operator=(const SolanaCompiledInstruction&) =
      delete;
  SolanaCompiledInstruction& operator=(SolanaCompiledInstruction&&);
  ~SolanaCompiledInstruction();

  bool operator==(const SolanaCompiledInstruction& ins) const;

  void Serialize(std::vector<uint8_t>* bytes) const;
  static absl::optional<SolanaCompiledInstruction> Deserialize(
      const std::vector<uint8_t>& bytes,
      size_t* bytes_index);

  static absl::optional<SolanaCompiledInstruction> FromInstruction(
      const SolanaInstruction& instruction,
      const std::vector<SolanaAddress>& static_accounts,
      const std::vector<SolanaMessageAddressTableLookup>& addr_table_lookups,
      uint8_t num_of_total_write_indexes);

  uint8_t program_id_index() const { return program_id_index_; }
  const std::vector<uint8_t>& data() const { return data_; }
  const std::vector<uint8_t>& account_indexes() const {
    return account_indexes_;
  }

  void SetProgramIdIndexForTesting(uint8_t program_id_index) {
    program_id_index_ = program_id_index;
  }
  void SetAccountIndexesForTesting(
      const std::vector<uint8_t>& account_indexes) {
    account_indexes_ = account_indexes;
  }

 private:
  // Index into the transaction keys array indicating the program account that
  // executes this instruction.
  uint8_t program_id_index_ = 0;
  // Ordered indices into the transaction keys array indicating which accounts
  // to pass to the program.
  std::vector<uint8_t> account_indexes_;
  // The program input data.
  std::vector<uint8_t> data_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_COMPILED_INSTRUCTION_H_
