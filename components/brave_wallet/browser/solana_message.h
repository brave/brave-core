/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_H_

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class SolanaMessage {
 public:
  SolanaMessage(const std::string& recent_blockhash,
                const std::string& fee_payer,
                const std::vector<SolanaInstruction>& instructions);
  SolanaMessage(const SolanaMessage&);
  ~SolanaMessage();

  absl::optional<std::vector<uint8_t>> Serialize(
      std::vector<std::string>* signers) const;

  void SetRecentBlockHash(const std::string& recent_blockhash) {
    recent_blockhash_ = recent_blockhash;
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaMessageUnitTest, GetUniqueAccountMetas);

  void GetUniqueAccountMetas(
      std::vector<SolanaAccountMeta>* unique_account_metas) const;

  std::string recent_blockhash_;
  // The account responsible for paying the cost of executing a transaction.
  std::string fee_payer_;
  std::vector<SolanaInstruction> instructions_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_H_
