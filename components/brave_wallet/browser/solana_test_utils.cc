/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_test_utils.h"

#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"
#include "brave/components/brave_wallet/browser/solana_message_header.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/solana_address.h"

namespace brave_wallet {

SolanaMessage GetTestLegacyMessage() {
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  SolanaInstruction instruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, std::nullopt, false, true)},
      data);
  auto message = SolanaMessage::CreateLegacyMessage(
      kRecentBlockhash, kLastValidBlockHeight, kFromAccount, {instruction});
  return std::move(*message);
}

SolanaMessage GetTestV0Message() {
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  SolanaMessageAddressTableLookup lookup(*SolanaAddress::FromBase58(kToAccount),
                                         {3, 1}, {2, 4});
  SolanaInstruction instruction(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kFromAccount, std::nullopt, true, true),
       SolanaAccountMeta(kToAccount, 1, false, true)},
      data);
  std::vector<SolanaMessageAddressTableLookup> lookups;
  lookups.push_back(std::move(lookup));
  return SolanaMessage(
      mojom::SolanaMessageVersion::kV0, kRecentBlockhash, kLastValidBlockHeight,
      kFromAccount, SolanaMessageHeader(1, 0, 1),
      {*SolanaAddress::FromBase58(kFromAccount),
       *SolanaAddress::FromBase58(mojom::kSolanaSystemProgramId)},
      {instruction}, std::move(lookups));
}

SolanaInstruction GetAdvanceNonceAccountInstruction() {
  uint32_t instruction_type = static_cast<uint32_t>(
      mojom::SolanaSystemInstruction::kAdvanceNonceAccount);
  instruction_type = base::ByteSwapToLE32(instruction_type);

  std::vector<uint8_t> instruction_data(
      reinterpret_cast<uint8_t*>(&instruction_type),
      reinterpret_cast<uint8_t*>(&instruction_type) + sizeof(instruction_type));

  return SolanaInstruction(
      mojom::kSolanaSystemProgramId,
      std::vector<SolanaAccountMeta>(
          {SolanaAccountMeta(kTestAccount, std::nullopt, false, true),
           SolanaAccountMeta(kToAccount, std::nullopt, false, false),
           SolanaAccountMeta(kFromAccount, std::nullopt, true, false)}),
      instruction_data);
}

}  // namespace brave_wallet
