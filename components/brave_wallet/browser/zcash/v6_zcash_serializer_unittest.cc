/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/v6_zcash_serializer.h"

#include <array>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// A simple v6 transaction with no inputs, outputs, or shielded pools.
ZCashTransaction MakeEmptyV6Tx() {
  ZCashTransaction tx;
  tx.set_consensus_brach_id(0xc2d6d0b4);
  tx.set_expiry_height(10000);
  tx.set_locktime(1);
  tx.v6_part() = ZCashTransaction::V6Part{};
  return tx;
}

}  // namespace

// PushHeader (private, accessible via FRIEND_TEST) must emit exactly 28 bytes:
// 4 (version) + 4 (group id) + 4 (branch id) + 4 (locktime) + 4 (expiry) +
// 8 (zip233_amount).  Changing zip233_amount must change the header hash.
TEST(ZCashV6SerializerTest, HashHeader) {
  auto tx = MakeEmptyV6Tx();

  // Inspect raw header bytes via FRIEND_TEST access to the private PushHeader.
  BtcLikeSerializerStream stream;
  ZCashV6Serializer::PushHeader(tx, stream);
  ASSERT_EQ(stream.data().size(), 28u);

  // v6 version constant (0x80000006) in little-endian.
  EXPECT_EQ(stream.data()[0], 0x06u);
  EXPECT_EQ(stream.data()[1], 0x00u);
  EXPECT_EQ(stream.data()[2], 0x00u);
  EXPECT_EQ(stream.data()[3], 0x80u);

  // zip233_amount = 0 occupies bytes 20-27 (all zero).
  for (size_t i = 20; i < 28; ++i) {
    EXPECT_EQ(stream.data()[i], 0x00u) << "zip233_amount byte " << i;
  }

  // A non-zero zip233_amount must change the header hash.
  auto tx2 = tx;
  tx2.v6_part()->zip233_amount = 100000;
  EXPECT_NE(ZCashV6Serializer::HashHeader(tx),
            ZCashV6Serializer::HashHeader(tx2));

  // Header bytes differ at exactly the zip233_amount position.
  BtcLikeSerializerStream stream2;
  ZCashV6Serializer::PushHeader(tx2, stream2);
  // Bytes 0-19 are identical.
  for (size_t i = 0; i < 20; ++i) {
    EXPECT_EQ(stream.data()[i], stream2.data()[i]) << "byte " << i;
  }
  // Bytes 20-27 differ (zip233_amount = 100000 = 0x00000000000186A0).
  EXPECT_NE(stream.data().at(20), stream2.data().at(20));
}

// SerializeRawTransaction must emit the correct wire format for four cases:
// transparent-only, legacy-orchard only, ironwood only, and both pools.
TEST(ZCashV6SerializerTest, SerializeRawTransaction) {
  // Case 1: transparent-only (zip233_amount = 0, no shielded pools).
  // Expected layout:
  //   header (28) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard cs(1) + ironwood cs(1) = 34 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 34u);

    // v6 version group id (0xD884B698) in LE at bytes 4-7.
    EXPECT_EQ(bytes[4], 0x98u);
    EXPECT_EQ(bytes[5], 0xB6u);
    EXPECT_EQ(bytes[6], 0x84u);
    EXPECT_EQ(bytes[7], 0xD8u);

    // zip233_amount = 0 at bytes 20-27.
    for (size_t i = 20; i < 28; ++i) {
      EXPECT_EQ(bytes[i], 0x00u) << "zip233_amount byte " << i;
    }

    // Both bundle slots are empty (compact-size 0x00).
    EXPECT_EQ(bytes[32], 0x00u);  // legacy_orchard empty
    EXPECT_EQ(bytes[33], 0x00u);  // ironwood empty
  }

  // Case 2: legacy_orchard.raw_tx set, ironwood empty.
  // Expected layout:
  //   header(28) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard(2) + ironwood cs(1) = 35 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    tx.v6_part()->legacy_orchard.raw_tx = std::vector<uint8_t>{0x01, 0x02};
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 35u);

    EXPECT_EQ(bytes[32], 0x01u);  // legacy_orchard[0]
    EXPECT_EQ(bytes[33], 0x02u);  // legacy_orchard[1]
    EXPECT_EQ(bytes[34], 0x00u);  // ironwood empty
  }

  // Case 3: ironwood.raw_tx set, legacy_orchard empty.
  // Expected layout:
  //   header(28) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard cs(1) + ironwood(3) = 36 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    tx.v6_part()->ironwood.raw_tx =
        std::vector<uint8_t>{0xAB, 0xCD, 0xEF};
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 36u);

    EXPECT_EQ(bytes[32], 0x00u);  // legacy_orchard empty
    EXPECT_EQ(bytes[33], 0xABu);  // ironwood[0]
    EXPECT_EQ(bytes[34], 0xCDu);  // ironwood[1]
    EXPECT_EQ(bytes[35], 0xEFu);  // ironwood[2]
  }

  // Case 4: both pools have raw_tx set (one byte each).
  // Expected layout:
  //   header(28) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard(1) + ironwood(1) = 34 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    tx.v6_part()->legacy_orchard.raw_tx = std::vector<uint8_t>{0x11};
    tx.v6_part()->ironwood.raw_tx = std::vector<uint8_t>{0x22};
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 34u);

    EXPECT_EQ(bytes[32], 0x11u);  // legacy_orchard[0]
    EXPECT_EQ(bytes[33], 0x22u);  // ironwood[0]
  }
}

// CalculateTxIdDigest must be deterministic and sensitive to pool digests and
// zip233_amount.
TEST(ZCashV6SerializerTest, CalculateTxIdDigest) {
  auto tx = MakeEmptyV6Tx();

  // Determinism.
  EXPECT_EQ(ZCashV6Serializer::CalculateTxIdDigest(tx),
            ZCashV6Serializer::CalculateTxIdDigest(tx));

  // zip233_amount is part of the digest.
  auto tx_nonzero_amount = tx;
  tx_nonzero_amount.v6_part()->zip233_amount = 1000;
  EXPECT_NE(ZCashV6Serializer::CalculateTxIdDigest(tx),
            ZCashV6Serializer::CalculateTxIdDigest(tx_nonzero_amount));

  // legacy_orchard digest is included independently.
  auto tx_legacy = tx;
  std::array<uint8_t, kZCashDigestSize> fake_digest{};
  fake_digest[0] = 0xFF;
  tx_legacy.v6_part()->legacy_orchard.digest = fake_digest;
  EXPECT_NE(ZCashV6Serializer::CalculateTxIdDigest(tx),
            ZCashV6Serializer::CalculateTxIdDigest(tx_legacy));

  // ironwood digest is included independently.
  auto tx_ironwood = tx;
  tx_ironwood.v6_part()->ironwood.digest = fake_digest;
  EXPECT_NE(ZCashV6Serializer::CalculateTxIdDigest(tx),
            ZCashV6Serializer::CalculateTxIdDigest(tx_ironwood));

  // The two pools contribute to different hash positions.
  EXPECT_NE(ZCashV6Serializer::CalculateTxIdDigest(tx_legacy),
            ZCashV6Serializer::CalculateTxIdDigest(tx_ironwood));

  // CalculateSignatureDigest is also sensitive to pool digests.
  EXPECT_NE(
      ZCashV6Serializer::CalculateSignatureDigest(tx, std::nullopt),
      ZCashV6Serializer::CalculateSignatureDigest(tx_legacy, std::nullopt));
  EXPECT_NE(
      ZCashV6Serializer::CalculateSignatureDigest(tx, std::nullopt),
      ZCashV6Serializer::CalculateSignatureDigest(tx_ironwood, std::nullopt));
}

// Exact byte-level differences between a v5 and equivalent v6 serialization:
//   - byte 0: 0x05 (v5) vs 0x06 (v6)
//   - bytes 4-7: group id 0x26A7270A vs 0xD884B698
//   - bytes 20-27 (v6 only): zip233_amount (8 bytes)
//   - trailing: v5 has one orchard slot; v6 has two (legacy_orchard + ironwood)
//   - v6.size() == v5.size() + 9
TEST(ZCashV6SerializerTest, DifferentialV5V6) {
  // Build a minimal transparent-only v5 transaction.
  ZCashTransaction v5_tx;
  v5_tx.set_consensus_brach_id(0xc2d6d0b4);
  v5_tx.set_expiry_height(10000);
  v5_tx.set_locktime(1);

  // Build an equivalent v6 transaction (same header fields, zip233_amount=0).
  auto v6_tx = v5_tx;
  v6_tx.v6_part() = ZCashTransaction::V6Part{};

  auto v5_bytes = ZCashV5Serializer::SerializeRawTransaction(v5_tx);
  auto v6_bytes = ZCashV6Serializer::SerializeRawTransaction(v6_tx);

  // v6 is exactly 9 bytes longer (8 for zip233_amount + 1 for ironwood slot).
  ASSERT_EQ(v6_bytes.size(), v5_bytes.size() + 9);
  ASSERT_EQ(v5_bytes.size(), 25u);
  ASSERT_EQ(v6_bytes.size(), 34u);

  // Version byte: v5 = 0x05, v6 = 0x06.
  EXPECT_EQ(v5_bytes[0], 0x05u);
  EXPECT_EQ(v6_bytes[0], 0x06u);

  // Version group id (LE):
  //   v5: 0x26A7270A → 0x0A 0x27 0xA7 0x26
  EXPECT_EQ(v5_bytes[4], 0x0Au);
  EXPECT_EQ(v5_bytes[5], 0x27u);
  EXPECT_EQ(v5_bytes[6], 0xA7u);
  EXPECT_EQ(v5_bytes[7], 0x26u);
  //   v6: 0xD884B698 → 0x98 0xB6 0x84 0xD8
  EXPECT_EQ(v6_bytes[4], 0x98u);
  EXPECT_EQ(v6_bytes[5], 0xB6u);
  EXPECT_EQ(v6_bytes[6], 0x84u);
  EXPECT_EQ(v6_bytes[7], 0xD8u);

  // Bytes 8-19 (consensus_branch_id, locktime, expiry_height) are identical.
  for (size_t i = 8; i < 20; ++i) {
    EXPECT_EQ(v5_bytes[i], v6_bytes[i]) << "common header byte " << i;
  }

  // v6 bytes 20-27: zip233_amount = 0 (all zero).
  for (size_t i = 20; i < 28; ++i) {
    EXPECT_EQ(v6_bytes[i], 0x00u) << "zip233_amount byte " << i;
  }

  // After the header, trailing fields (inputs cs, outputs cs, sapling) match.
  // v5: starts at 20, v6: starts at 28. First 4 bytes are identical.
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(v5_bytes[20 + i], v6_bytes[28 + i])
        << "trailing field byte " << i;
  }
  // v5 has one bundle slot (orchard = 0x00 at byte 24).
  EXPECT_EQ(v5_bytes[24], 0x00u);
  // v6 has two bundle slots (legacy_orchard = 0x00 at byte 32, ironwood at 33).
  EXPECT_EQ(v6_bytes[32], 0x00u);
  EXPECT_EQ(v6_bytes[33], 0x00u);
}

// When no pool digest is set, the serializer implicitly uses
// Blake2b256({}, "ZTxIdOrchardHash") for both legacy_orchard and ironwood.
// Explicitly setting both digests to that same value must produce identical
// txid and signature digests.
TEST(ZCashV6SerializerTest, EmptyFallbackInvariant) {
  auto fallback = ZCashSerializer::Blake2b256(
      {}, base::byte_span_from_cstring("ZTxIdOrchardHash"));

  // TxId digest.
  {
    auto tx_implicit = MakeEmptyV6Tx();
    auto txid_implicit = ZCashV6Serializer::CalculateTxIdDigest(tx_implicit);

    auto tx_explicit = MakeEmptyV6Tx();
    tx_explicit.v6_part()->legacy_orchard.digest = fallback;
    tx_explicit.v6_part()->ironwood.digest = fallback;
    auto txid_explicit = ZCashV6Serializer::CalculateTxIdDigest(tx_explicit);

    EXPECT_EQ(txid_implicit, txid_explicit);
  }

  // Signature digest.
  {
    auto tx_implicit = MakeEmptyV6Tx();
    auto sigdigest_implicit =
        ZCashV6Serializer::CalculateSignatureDigest(tx_implicit, std::nullopt);

    auto tx_explicit = MakeEmptyV6Tx();
    tx_explicit.v6_part()->legacy_orchard.digest = fallback;
    tx_explicit.v6_part()->ironwood.digest = fallback;
    auto sigdigest_explicit =
        ZCashV6Serializer::CalculateSignatureDigest(tx_explicit, std::nullopt);

    EXPECT_EQ(sigdigest_implicit, sigdigest_explicit);
  }

  // Changing one pool's digest to a non-fallback value must break the invariant.
  {
    auto tx_modified = MakeEmptyV6Tx();
    std::array<uint8_t, kZCashDigestSize> different_digest = fallback;
    different_digest[0] ^= 0xFF;
    tx_modified.v6_part()->legacy_orchard.digest = different_digest;
    tx_modified.v6_part()->ironwood.digest = fallback;

    auto tx_explicit = MakeEmptyV6Tx();
    tx_explicit.v6_part()->legacy_orchard.digest = fallback;
    tx_explicit.v6_part()->ironwood.digest = fallback;

    EXPECT_NE(ZCashV6Serializer::CalculateTxIdDigest(tx_modified),
              ZCashV6Serializer::CalculateTxIdDigest(tx_explicit));
  }
}

}  // namespace brave_wallet
