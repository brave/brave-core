/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_HEADER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_HEADER_H_

#include <cstdint>
#include <optional>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

// https://docs.solana.com/developing/programming-model/transactions#message-header-format
struct SolanaMessageHeader {
  SolanaMessageHeader(uint8_t num_required_signatures,
                      uint8_t num_readonly_signed_accounts,
                      uint8_t num_readonly_unsigned_accounts);
  SolanaMessageHeader() = default;
  ~SolanaMessageHeader() = default;
  bool operator==(const SolanaMessageHeader&) const;

  base::Value::Dict ToValue() const;
  static std::optional<SolanaMessageHeader> FromValue(
      const base::Value::Dict& value);
  mojom::SolanaMessageHeaderPtr ToMojom() const;
  static SolanaMessageHeader FromMojom(
      const mojom::SolanaMessageHeaderPtr& mojom_msg_header);

  uint8_t num_required_signatures = 0;
  uint8_t num_readonly_signed_accounts = 0;
  uint8_t num_readonly_unsigned_accounts = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_HEADER_H_
