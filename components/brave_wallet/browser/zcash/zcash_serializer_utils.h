/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_UTILS_H_

#include <array>
#include <optional>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

// Shared ZIP-244 primitives used by v5 and v6 serializers.
class ZCashSerializerUtils {
 public:
  static std::array<uint8_t, kZCashDigestSize> CalculateSignatureDigest(
      const ZCashTransaction& zcash_transaction,
      const std::optional<ZCashTransaction::TxInput>& input);

  static std::array<uint8_t, kZCashDigestSize> Blake2b256(
      base::span<const uint8_t> payload,
      base::span<const uint8_t, kBlake2bPersonalizerLength> personalizer);

  static void PushOutpoint(const ZCashTransaction::Outpoint& outpoint,
                           BtcLikeSerializerStream& stream);
  static void PushOutput(const ZCashTransaction::TxOutput& output,
                         BtcLikeSerializerStream& stream);
  static void SerializeTransparentInputs(const ZCashTransaction& tx,
                                         BtcLikeSerializerStream& stream);
  static void SerializeTransparentOutputs(const ZCashTransaction& tx,
                                          BtcLikeSerializerStream& stream);

  // ZIP-244 transparent digests (shared, version-independent).
  static std::array<uint8_t, kZCashDigestSize> HashTransparentTxId(
      const ZCashTransaction& tx);
  static std::array<uint8_t, kZCashDigestSize> HashTransparentSignature(
      const ZCashTransaction& tx,
      const std::optional<ZCashTransaction::TxInput>& input);

  // "ZcashTxHash_" || consensus_branch_id — the top-level combiner
  // personalizer.
  static std::array<uint8_t, kBlake2bPersonalizerLength> GetHashPersonalizer(
      const ZCashTransaction& tx);

 private:
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerUtilsTest, HashOutputs);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerUtilsTest, HashPrevouts);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerUtilsTest, HashSequences);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerUtilsTest, HashTxIn);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerUtilsTest, HashAmounts);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerUtilsTest, HashScriptPubKeys);

  static std::array<uint8_t, kZCashDigestSize> HashPrevouts(
      const ZCashTransaction& tx);
  static std::array<uint8_t, kZCashDigestSize> HashSequences(
      const ZCashTransaction& tx);
  static std::array<uint8_t, kZCashDigestSize> HashOutputs(
      const ZCashTransaction& tx);
  static std::array<uint8_t, kZCashDigestSize> HashAmounts(
      const ZCashTransaction& tx);
  static std::array<uint8_t, kZCashDigestSize> HashScriptPubKeys(
      const ZCashTransaction& tx);
  static std::array<uint8_t, kZCashDigestSize> HashTxIn(
      std::optional<ZCashTransaction::TxInput> tx_in);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_UTILS_H_
