/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

// Instruction specifies a single program, a subset of the transaction's
// accounts that should be passed to the program, and a data byte array that is
// passed to the program. See
// https://docs.solana.com/developing/programming-model/transactions#instructions
// for more details.
class SolanaInstruction {
 public:
  SolanaInstruction(const std::string& program_id,
                    std::vector<SolanaAccountMeta>&& accounts,
                    const std::vector<uint8_t>& data);
  ~SolanaInstruction();

  SolanaInstruction(const SolanaInstruction&);
  bool operator==(const SolanaInstruction&) const;

  bool Serialize(const std::vector<SolanaAccountMeta>& message_account_metas,
                 std::vector<uint8_t>* bytes) const;

  const std::vector<SolanaAccountMeta>& GetAccounts() const {
    return accounts_;
  }
  const std::string& GetProgramId() const { return program_id_; }

  mojom::SolanaInstructionPtr ToMojomSolanaInstruction() const;
  base::Value ToValue() const;

  static void FromMojomSolanaInstructions(
      const std::vector<mojom::SolanaInstructionPtr>& mojom_instructions,
      std::vector<SolanaInstruction>* instructions);
  static absl::optional<SolanaInstruction> FromValue(const base::Value& value);

 private:
  std::string program_id_;
  std::vector<SolanaAccountMeta> accounts_;
  std::vector<uint8_t> data_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_
