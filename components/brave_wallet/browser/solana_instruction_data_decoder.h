/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_DATA_DECODER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_DATA_DECODER_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_instruction_decoded_data.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet::solana_ins_data_decoder {

absl::optional<SolanaInstructionDecodedData> Decode(
    const std::vector<uint8_t>& data,
    const std::string& program_id);

std::vector<InsParamPair> GetAccountParamsForTesting(
    absl::optional<mojom::SolanaSystemInstruction> sys_ins_type,
    absl::optional<mojom::SolanaTokenInstruction> token_ins_type);

std::vector<mojom::SolanaInstructionAccountParamPtr>
GetMojomAccountParamsForTesting(
    absl::optional<mojom::SolanaSystemInstruction> sys_ins_type,
    absl::optional<mojom::SolanaTokenInstruction> token_ins_type);

}  // namespace brave_wallet::solana_ins_data_decoder

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_DATA_DECODER_H_
