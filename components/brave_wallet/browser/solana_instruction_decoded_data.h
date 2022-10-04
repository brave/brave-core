/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_DECODED_DATA_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_DECODED_DATA_H_

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// Pair of param name and it's localized name.
using InsParamPair = std::pair<std::string, std::string>;
// Tuple of param name, localized name, and value.
using InsParamTuple = std::tuple<std::string, std::string, std::string>;
using InsTypeAndParamTuple =
    std::tuple<absl::optional<mojom::SolanaSystemInstruction>,
               absl::optional<mojom::SolanaTokenInstruction>,
               std::vector<InsParamTuple>,  // Instruction params in data.
               std::vector<InsParamPair>>;  // Account params.

struct SolanaInstructionDecodedData {
  SolanaInstructionDecodedData();
  SolanaInstructionDecodedData(const SolanaInstructionDecodedData&);
  SolanaInstructionDecodedData(SolanaInstructionDecodedData&&);
  SolanaInstructionDecodedData& operator=(const SolanaInstructionDecodedData&);
  SolanaInstructionDecodedData& operator=(SolanaInstructionDecodedData&&);
  ~SolanaInstructionDecodedData();

  bool operator==(const SolanaInstructionDecodedData&) const;

  static absl::optional<SolanaInstructionDecodedData> FromMojom(
      const std::string& program_id,
      const mojom::DecodedSolanaInstructionDataPtr& mojom_decoded_data);
  mojom::DecodedSolanaInstructionDataPtr ToMojom() const;

  static absl::optional<SolanaInstructionDecodedData> FromValue(
      const base::Value::Dict& value);
  absl::optional<base::Value::Dict> ToValue() const;

  // There should be only one type that has value.
  bool IsValid() const { return !sys_ins_type != !token_ins_type; }

  absl::optional<mojom::SolanaSystemInstruction> sys_ins_type;
  absl::optional<mojom::SolanaTokenInstruction> token_ins_type;
  std::vector<InsParamTuple> params;
  std::vector<InsParamPair> account_params;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_DECODED_DATA_H_
