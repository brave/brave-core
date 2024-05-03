/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction_decoded_data.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class SolanaAddress;
class SolanaCompiledInstruction;
class SolanaMessageAddressTableLookup;
struct SolanaMessageHeader;

// Instruction specifies a single program, a subset of the transaction's
// accounts that should be passed to the program, and a data byte array that is
// passed to the program. See
// https://docs.solana.com/developing/programming-model/transactions#instructions
// for more details.
class SolanaInstruction {
 public:
  SolanaInstruction(const std::string& program_id,
                    std::vector<SolanaAccountMeta>&& accounts,
                    base::span<const uint8_t> data);
  SolanaInstruction(const std::string& program_id,
                    std::vector<SolanaAccountMeta>&& accounts,
                    base::span<const uint8_t> data,
                    std::optional<SolanaInstructionDecodedData> decoded_data);
  ~SolanaInstruction();

  SolanaInstruction(const SolanaInstruction&);
  SolanaInstruction(SolanaInstruction&&);
  SolanaInstruction& operator=(const SolanaInstruction&);
  SolanaInstruction& operator=(SolanaInstruction&&);
  bool operator==(const SolanaInstruction&) const;

  static std::optional<SolanaInstruction> FromCompiledInstruction(
      const SolanaCompiledInstruction& compiled_instruction,
      const SolanaMessageHeader& message_header,
      const std::vector<SolanaAddress>& static_accounts,
      const std::vector<SolanaMessageAddressTableLookup>& addr_table_lookups,
      uint8_t num_of_write_indexes,
      uint8_t num_of_read_indexes);

  const std::vector<SolanaAccountMeta>& GetAccounts() const {
    return accounts_;
  }
  const std::string& GetProgramId() const { return program_id_; }
  const std::vector<uint8_t>& data() const { return data_; }

  mojom::SolanaInstructionPtr ToMojomSolanaInstruction() const;
  base::Value::Dict ToValue() const;

  static void FromMojomSolanaInstructions(
      const std::vector<mojom::SolanaInstructionPtr>& mojom_instructions,
      std::vector<SolanaInstruction>* instructions);
  static std::optional<SolanaInstruction> FromValue(
      const base::Value::Dict& value);

 private:
  std::string program_id_;
  std::vector<SolanaAccountMeta> accounts_;
  std::vector<uint8_t> data_;

  std::optional<SolanaInstructionDecodedData> decoded_data_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_
