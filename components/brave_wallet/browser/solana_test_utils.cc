/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_test_utils.h"

#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
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
  return SolanaInstruction(
      mojom::kSolanaSystemProgramId,
      std::vector<SolanaAccountMeta>(
          {SolanaAccountMeta(kTestAccount, std::nullopt, false, true),
           SolanaAccountMeta(kToAccount, std::nullopt, false, false),
           SolanaAccountMeta(kFromAccount, std::nullopt, true, false)}),
      base::byte_span_from_ref(
          base::U32FromLittleEndian(base::byte_span_from_ref(
              mojom::SolanaSystemInstruction::kAdvanceNonceAccount))));
}

}  // namespace brave_wallet
