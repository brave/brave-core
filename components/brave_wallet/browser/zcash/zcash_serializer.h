/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_

#include <array>
#include <optional>
#include <vector>

#include "base/functional/function_ref.h"
#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"

namespace brave_wallet {

// Implements algorithms for calculating tx id, tx signature digest
// https://zips.z.cash/zip-0244 (v5) / ZIP-246 (v6)
// Dispatch entry points route to ZCashV5Serializer or ZCashV6Serializer
// depending on whether tx.v6_part() is set.
class ZCashSerializer {
 public:
  // Dispatch entry points (route on tx.v6_part().has_value()).
  static std::array<uint8_t, kZCashDigestSize> CalculateTxIdDigest(
      const ZCashTransaction& zcash_transaction);
  static std::array<uint8_t, kZCashDigestSize> CalculateSignatureDigest(
      const ZCashTransaction& zcash_transaction,
      const std::optional<ZCashTransaction::TxInput>& input);
  static std::vector<uint8_t> SerializeRawTransaction(
      const ZCashTransaction& zcash_transaction);
  static bool SignTransparentPart(KeyringService& keyring_service,
                                  const mojom::AccountIdPtr& account_id,
                                  ZCashTransaction& tx);

  // Shared primitives used by v5 and v6.
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

  static void SerializeSignature(const ZCashTransaction& tx,
                                 ZCashTransaction::TxInput& input,
                                 const std::vector<uint8_t>& pubkey,
                                 const std::vector<uint8_t>& signature);

  // Shared transparent signing loop; the version supplies the per-input
  // signature digest (v5 vs v6 differ only in that digest).
  static bool SignTransparentPart(
      KeyringService& keyring_service,
      const mojom::AccountIdPtr& account_id,
      ZCashTransaction& tx,
      base::FunctionRef<std::array<uint8_t, kZCashDigestSize>(
          const ZCashTransaction&,
          const ZCashTransaction::TxInput&)> compute_sig_digest);

 private:
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashOutputs);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashPrevouts);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashSequences);
  FRIEND_TEST_ALL_PREFIXES(ZCashSerializerTest, HashTxIn);

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

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SERIALIZER_H_
