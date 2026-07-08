/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_V6_ZCASH_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_V6_ZCASH_SERIALIZER_H_

#include <array>
#include <optional>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

// ZCash v6 (Ironwood / NU7) transaction serialization.
// Only reachable when tx.v6_part() is set, gated behind
// IsZCashIronwoodTransactionEnabled().
class ZCashV6Serializer {
 public:
  static std::array<uint8_t, kZCashDigestSize> CalculateTxIdDigest(
      const ZCashTransaction& tx);
  static std::array<uint8_t, kZCashDigestSize> CalculateSignatureDigest(
      const ZCashTransaction& tx,
      const std::optional<ZCashTransaction::TxInput>& input);
  static std::vector<uint8_t> SerializeRawTransaction(
      const ZCashTransaction& tx);
  static bool SignTransparentPart(KeyringService& keyring_service,
                                  const mojom::AccountIdPtr& account_id,
                                  ZCashTransaction& tx);

 private:
  FRIEND_TEST_ALL_PREFIXES(ZCashV6SerializerTest, HashHeader);
  FRIEND_TEST_ALL_PREFIXES(ZCashV6SerializerTest, SerializeRawTransaction);
  FRIEND_TEST_ALL_PREFIXES(ZCashV6SerializerTest, CalculateTxIdDigest);
  FRIEND_TEST_ALL_PREFIXES(ZCashV6SerializerTest, DifferentialV5V6);
  FRIEND_TEST_ALL_PREFIXES(ZCashV6SerializerTest, EmptyFallbackInvariant);

  static void PushHeader(const ZCashTransaction& tx,
                         BtcLikeSerializerStream& stream);
  static std::array<uint8_t, kZCashDigestSize> HashHeader(
      const ZCashTransaction& tx);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_V6_ZCASH_SERIALIZER_H_
