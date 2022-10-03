/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_decoded_data.h"

namespace brave_wallet {

SolanaInstructionDecodedData::SolanaInstructionDecodedData() = default;
SolanaInstructionDecodedData::SolanaInstructionDecodedData(
    const SolanaInstructionDecodedData&) = default;
SolanaInstructionDecodedData& SolanaInstructionDecodedData::operator=(
    const SolanaInstructionDecodedData&) = default;
SolanaInstructionDecodedData::SolanaInstructionDecodedData(
    SolanaInstructionDecodedData&&) = default;
SolanaInstructionDecodedData& SolanaInstructionDecodedData::operator=(
    SolanaInstructionDecodedData&&) = default;
SolanaInstructionDecodedData::~SolanaInstructionDecodedData() = default;

bool SolanaInstructionDecodedData::operator==(
    const SolanaInstructionDecodedData& other) const {
  return sys_ins_type == other.sys_ins_type &&
         token_ins_type == other.token_ins_type && params == other.params &&
         account_params == other.account_params;
}

}  // namespace brave_wallet
