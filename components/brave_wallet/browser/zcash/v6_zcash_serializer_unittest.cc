/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/v6_zcash_serializer.h"

#include <array>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer_utils.h"
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
  tx.init_v6_part();
  return tx;
}

}  // namespace

// PushHeader (private, accessible via FRIEND_TEST) must emit exactly 20 bytes:
// 4 (version) + 4 (group id) + 4 (branch id) + 4 (locktime) + 4 (expiry).
// Per ZIP 229 the v6 header ends at expiryHeight; there is NO zip233Amount
// field (that only existed in the withdrawn ZIP 230).
TEST(ZCashV6SerializerTest, HashHeader) {
  auto tx = MakeEmptyV6Tx();

  // Inspect raw header bytes via FRIEND_TEST access to the private
  // PushHeader.
  BtcLikeSerializerStream stream;
  ZCashV6Serializer::PushHeader(tx, stream);
  ASSERT_EQ(stream.data().size(), 20u);

  // v6 version constant (0x80000006) in little-endian.
  EXPECT_EQ(stream.data()[0], 0x06u);
  EXPECT_EQ(stream.data()[1], 0x00u);
  EXPECT_EQ(stream.data()[2], 0x00u);
  EXPECT_EQ(stream.data()[3], 0x80u);

  // v6 version group id (0xD884B698) in LE at bytes 4-7.
  EXPECT_EQ(stream.data()[4], 0x98u);
  EXPECT_EQ(stream.data()[5], 0xB6u);
  EXPECT_EQ(stream.data()[6], 0x84u);
  EXPECT_EQ(stream.data()[7], 0xD8u);
  // The header hash is deterministic for identical header fields.
  EXPECT_EQ(
      "0xe2a3668ae60a6327e24ae6fcd7a4775c9da93148663f14f0552dc2bfecf76aa8",
      ToHex(ZCashV6Serializer::HashHeader(tx)));
}

// SerializeRawTransaction must emit the correct wire format for four cases:
// transparent-only, legacy-orchard only, ironwood only, and both pools.
TEST(ZCashV6SerializerTest, SerializeRawTransaction) {
  // Case 1: transparent-only (no shielded pools).
  // Expected layout:
  //   header (20) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard cs(1) + ironwood cs(1) = 26 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 26u);

    // v6 version group id (0xD884B698) in LE at bytes 4-7.
    EXPECT_EQ(bytes[4], 0x98u);
    EXPECT_EQ(bytes[5], 0xB6u);
    EXPECT_EQ(bytes[6], 0x84u);
    EXPECT_EQ(bytes[7], 0xD8u);

    // The header ends at byte 19; the transparent/sapling counts follow
    // immediately (no zip233Amount). All zero for an empty tx.
    for (size_t i = 20; i < 24; ++i) {
      EXPECT_EQ(bytes[i], 0x00u) << "empty count byte " << i;
    }

    // Both bundle slots are empty (compact-size 0x00).
    EXPECT_EQ(bytes[24], 0x00u);  // legacy_orchard empty
    EXPECT_EQ(bytes[25], 0x00u);  // ironwood empty
  }

  // Case 2: legacy_orchard.raw_tx set, ironwood empty.
  // Expected layout:
  //   header(20) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard(2) + ironwood cs(1) = 27 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    tx.v6_part().legacy_orchard.raw_tx = std::vector<uint8_t>{0x01, 0x02};
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 27u);

    EXPECT_EQ(bytes[24], 0x01u);  // legacy_orchard[0]
    EXPECT_EQ(bytes[25], 0x02u);  // legacy_orchard[1]
    EXPECT_EQ(bytes[26], 0x00u);  // ironwood empty
  }

  // Case 3: ironwood.raw_tx set, legacy_orchard empty.
  // Expected layout:
  //   header(20) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard cs(1) + ironwood(3) = 28 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    tx.v6_part().ironwood.raw_tx = std::vector<uint8_t>{0xAB, 0xCD, 0xEF};
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 28u);

    EXPECT_EQ(bytes[24], 0x00u);  // legacy_orchard empty
    EXPECT_EQ(bytes[25], 0xABu);  // ironwood[0]
    EXPECT_EQ(bytes[26], 0xCDu);  // ironwood[1]
    EXPECT_EQ(bytes[27], 0xEFu);  // ironwood[2]
  }

  // Case 4: both pools have raw_tx set (one byte each).
  // Expected layout:
  //   header(20) + inputs cs(1) + outputs cs(1) + sapling(2) +
  //   legacy_orchard(1) + ironwood(1) = 26 bytes.
  {
    auto tx = MakeEmptyV6Tx();
    tx.v6_part().legacy_orchard.raw_tx = std::vector<uint8_t>{0x11};
    tx.v6_part().ironwood.raw_tx = std::vector<uint8_t>{0x22};
    auto bytes = ZCashV6Serializer::SerializeRawTransaction(tx);
    ASSERT_EQ(bytes.size(), 26u);

    EXPECT_EQ(bytes[24], 0x11u);  // legacy_orchard[0]
    EXPECT_EQ(bytes[25], 0x22u);  // ironwood[0]
  }
}

// CalculateTxIdDigest must be deterministic and sensitive to pool digests.
TEST(ZCashV6SerializerTest, CalculateTxIdDigest) {
  auto tx = MakeEmptyV6Tx();

  // Determinism.
  EXPECT_EQ(
      "0x59b9ff363b8d9a7fb16b9cf560835ddeb590dface54cb03388775b8fceff2ff3",
      ToHex(ZCashV6Serializer::CalculateTxIdDigest(tx)));

  // legacy_orchard digest is included independently.
  auto tx_legacy = tx;
  std::array<uint8_t, kZCashDigestSize> fake_digest{};
  fake_digest[0] = 0xFF;
  tx_legacy.v6_part().legacy_orchard.digest = fake_digest;
  EXPECT_NE(ZCashV6Serializer::CalculateTxIdDigest(tx),
            ZCashV6Serializer::CalculateTxIdDigest(tx_legacy));

  // ironwood digest is included independently.
  auto tx_ironwood = tx;
  tx_ironwood.v6_part().ironwood.digest = fake_digest;
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

// Exact byte-level differences between a v5 and equivalent v6 serialization.
// Both headers are 20 bytes (no zip233Amount in v6 per ZIP 229), so v6 differs
// from v5 only by:
//   - byte 0: 0x05 (v5) vs 0x06 (v6)
//   - bytes 4-7: group id 0x26A7270A vs 0xD884B698
//   - trailing: v5 has one orchard slot; v6 has two (legacy_orchard +
//     ironwood)
//   - v6.size() == v5.size() + 1
TEST(ZCashV6SerializerTest, DifferentialV5V6) {
  // Build a minimal transparent-only v5 transaction.
  ZCashTransaction v5_tx;
  v5_tx.init_v5_part();
  v5_tx.set_consensus_brach_id(0xc2d6d0b4);
  v5_tx.set_expiry_height(10000);
  v5_tx.set_locktime(1);

  // Build an equivalent v6 transaction (same header fields).
  auto v6_tx = v5_tx;
  v6_tx.init_v6_part();

  auto v5_bytes = ZCashV5Serializer::SerializeRawTransaction(v5_tx);
  auto v6_bytes = ZCashV6Serializer::SerializeRawTransaction(v6_tx);

  // v6 is exactly 1 byte longer (the extra empty ironwood bundle slot).
  ASSERT_EQ(v6_bytes.size(), v5_bytes.size() + 1);
  ASSERT_EQ(v5_bytes.size(), 25u);
  ASSERT_EQ(v6_bytes.size(), 26u);

  // Version byte: v5 = 0x05, v6 = 0x06.
  EXPECT_EQ(v5_bytes[0], 0x05u);
  EXPECT_EQ(v6_bytes[0], 0x06u);

  // Version group id (LE):
  //   v5: 0x26A7270A -> 0x0A 0x27 0xA7 0x26
  EXPECT_EQ(v5_bytes[4], 0x0Au);
  EXPECT_EQ(v5_bytes[5], 0x27u);
  EXPECT_EQ(v5_bytes[6], 0xA7u);
  EXPECT_EQ(v5_bytes[7], 0x26u);
  //   v6: 0xD884B698 -> 0x98 0xB6 0x84 0xD8
  EXPECT_EQ(v6_bytes[4], 0x98u);
  EXPECT_EQ(v6_bytes[5], 0xB6u);
  EXPECT_EQ(v6_bytes[6], 0x84u);
  EXPECT_EQ(v6_bytes[7], 0xD8u);

  // Bytes 8-19 (consensus_branch_id, locktime, expiry_height) are identical.
  for (size_t i = 8; i < 20; ++i) {
    EXPECT_EQ(v5_bytes[i], v6_bytes[i]) << "common header byte " << i;
  }

  // Both headers end at byte 19; the transparent/sapling counts (4 bytes)
  // then match one-to-one at the same offsets.
  for (size_t i = 20; i < 24; ++i) {
    EXPECT_EQ(v5_bytes[i], v6_bytes[i]) << "trailing field byte " << i;
  }
  // v5 has one bundle slot (orchard = 0x00 at byte 24).
  EXPECT_EQ(v5_bytes[24], 0x00u);
  // v6 has two bundle slots (legacy_orchard = 0x00 at byte 24, ironwood at
  // 25).
  EXPECT_EQ(v6_bytes[24], 0x00u);
  EXPECT_EQ(v6_bytes[25], 0x00u);
}

// When no pool digest is set, the serializer implicitly uses the
// pool-specific v6 empty-bundle personalizer: "ZTxIdOrchardH_v6" for
// legacy_orchard and the distinct "ZTxIdIronwd_H_v6" for ironwood (matching
// orchard commitments.rs). Explicitly setting each pool's digest to its own
// fallback value must produce identical txid and signature digests.
TEST(ZCashV6SerializerTest, EmptyFallbackInvariant) {
  auto orchard_fallback = ZCashSerializerUtils::Blake2b256(
      {}, base::byte_span_from_cstring("ZTxIdOrchardH_v6"));
  auto ironwood_fallback = ZCashSerializerUtils::Blake2b256(
      {}, base::byte_span_from_cstring("ZTxIdIronwd_H_v6"));

  // The two pools must use DISTINCT personalizers, so their empty digests
  // differ.
  EXPECT_NE(orchard_fallback, ironwood_fallback);

  // TxId digest.
  {
    auto tx_implicit = MakeEmptyV6Tx();
    auto txid_implicit = ZCashV6Serializer::CalculateTxIdDigest(tx_implicit);

    auto tx_explicit = MakeEmptyV6Tx();
    tx_explicit.v6_part().legacy_orchard.digest = orchard_fallback;
    tx_explicit.v6_part().ironwood.digest = ironwood_fallback;
    auto txid_explicit = ZCashV6Serializer::CalculateTxIdDigest(tx_explicit);

    EXPECT_EQ(txid_implicit, txid_explicit);
  }

  // Signature digest.
  {
    auto tx_implicit = MakeEmptyV6Tx();
    auto sigdigest_implicit =
        ZCashV6Serializer::CalculateSignatureDigest(tx_implicit, std::nullopt);

    auto tx_explicit = MakeEmptyV6Tx();
    tx_explicit.v6_part().legacy_orchard.digest = orchard_fallback;
    tx_explicit.v6_part().ironwood.digest = ironwood_fallback;
    auto sigdigest_explicit =
        ZCashV6Serializer::CalculateSignatureDigest(tx_explicit, std::nullopt);

    EXPECT_EQ(sigdigest_implicit, sigdigest_explicit);
  }

  // Changing one pool's digest to a non-fallback value must break the
  // invariant.
  {
    auto tx_modified = MakeEmptyV6Tx();
    std::array<uint8_t, kZCashDigestSize> different_digest = orchard_fallback;
    different_digest[0] ^= 0xFF;
    tx_modified.v6_part().legacy_orchard.digest = different_digest;
    tx_modified.v6_part().ironwood.digest = ironwood_fallback;

    auto tx_explicit = MakeEmptyV6Tx();
    tx_explicit.v6_part().legacy_orchard.digest = orchard_fallback;
    tx_explicit.v6_part().ironwood.digest = ironwood_fallback;

    EXPECT_NE(ZCashV6Serializer::CalculateTxIdDigest(tx_modified),
              ZCashV6Serializer::CalculateTxIdDigest(tx_explicit));
  }
}

// Regression test built from a captured real testnet o->t (orchard ->
// transparent) transaction (see zcash_create_orchard_to_transparent_
// transaction_task.cc / zcash_complete_transaction_task.cc for how these
// fields are populated). Exercises the actual serializer end-to-end against
// values that produced a valid v6 transaction, rather than synthetic zeros.
TEST(ZCashV6SerializerTest, RealOrchardToTransparentTransaction) {
  ZCashTransaction tx;
  tx.init_v6_part();
  tx.set_consensus_brach_id(933566043u);
  tx.set_locktime(3447694u);
  tx.set_expiry_height(3447714u);

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "t1Xgp6z3tPYgsAtC6aA87ntNeTTGtKgUJvB";
    tx_output.amount = 10000u;
    tx_output.script_pubkey =
        PrefixedHexStringToBytes(
            "0x76a914978748a668db7815805585c56f981a316e220b0b88ac")
            .value();
    tx.transparent_part().outputs.push_back(std::move(tx_output));
  }

  auto& legacy_orchard = tx.v6_part().legacy_orchard;
  legacy_orchard.anchor_block_height = 3447688u;
  EXPECT_TRUE(PrefixedHexStringToFixed(
      "0x0bf9f62bce9e60fe3808e53e15eb341d13378f3491935c78c41d55994a279baa",
      legacy_orchard.digest.emplace()));
  legacy_orchard.raw_tx = PrefixedHexStringToBytes(
      "0x029239d4614397342b8b6891f41bfb7f0268c149a15da3ee5897d2342219b8660a44ff"
      "ea6f98"
      "cf2d84ea73fe9b3e0fd483812bba854512587df7eed0c244a47f227f6cecc81b8b9dd6e1"
      "5f2d"
      "1c523ced7e1d8944373380bd2ac12c469ea487909a99bc68a81bd3439e6be8b39557b595"
      "a2e0"
      "23338089f7cb74722e9cd4973b6a15dcef11c452a0154a68581e256f40a6d0aa145e2721"
      "3dfb"
      "98a6b3da7679a6b4176e667da8d23f9998d3851e77bf8821a99d56e1339047999521d8b9"
      "4e67"
      "098f8715662e5dfc59f4ed7b088823571d98127cc5860b37df10fd1a8971837c285e50c8"
      "e549"
      "bc7406b149ceba67ddfc344ff57ace532a5e8b4ffcd9bfdf4e335d42d1332aac11140636"
      "b09c"
      "636ae21f83781d282010d982fe7df369d0573e43f6fb51fb434d96eb02a2c7d372a1b134"
      "4f8d"
      "56c641ae3bc5ccfb884dface9d11b60cb4f67cdec97dd5ed26a94339a5881b37e5684876"
      "a8b6"
      "ca47d0c70164b2c8acf0f7f4d616c992cf8279cc8754b1d630b4c539d8281b291041e374"
      "cec5"
      "294e9a479454d8f14fcbfd93f801b9a3488199de50414075799c30f701bef38451fc8147"
      "0069"
      "b1045ebacee0f33ae67794d7bf1d6fdca01bee33f7be8f301c3cf67ce02620dbbaeb9beb"
      "f6aa"
      "5db5ed02a69e9fd33eca5d7107f52927d45afe806b3e20b2f62b0de497ecccbf8bc4f45e"
      "9693"
      "4de3ecf1047b04194ec9880b895191f2d5319a3b6830b535b2cd11cb44ef4517cd20d1ac"
      "8530"
      "acc2357cb99ae9d8be157954c5eb161ef63667662a6a3c4f7aae600a40a68806c02d9516"
      "09f9"
      "079aadb374c23ed1737b1e2560b51de869c6ea8e4d1b794930169a4ea277b4b40a6a7424"
      "554b"
      "90261cd02020426e7c5f4f1384c37653b91c724e0e20d59d5b9d7bc796747b510247fef4"
      "888e"
      "6de2e5ac7f5f746313d4caa85171a8e1247c8c3bee65b91aabb27c46d4e484441387ae4d"
      "17e2"
      "cfc4499a104269cb66643fe4e04cb5c0ba65abf4dde4f982908c05263cae198dad13258a"
      "7501"
      "92d3b857c19728759765784ea2d078cae77cf21887b86020c62a95a3a66a3458293a2286"
      "eb9d"
      "d9323c4dd5ceb80aab06c1ffd3744e8648131c44e0c9e76c3bc2f4e64c3d0855071c9135"
      "5c9c"
      "45d9b0037ca7bf62067a3fd7efad651c6e91d223dd16e6bec1c63516ee4ee897a7851251"
      "5f07"
      "4c564c90aebe9ec798c12ee4279d806d174b1ab5d651b681cee9c17072d61a36a0dbca59"
      "5df8"
      "719b73bef9c14757a29727d38b5fe78bb20a3715bb7ad6c09c9577506a12682879f1b404"
      "5929"
      "6b9aa00e06ce396b9145209b443d479b732e00dc72caca5d27bbcea557f6fd21c7869d75"
      "193f"
      "fddffebad25fcf52b177be3caf7b4d40a3220bb2dfa123996f6ec0f8d46902c02a97f2ae"
      "177d"
      "9c66711ddacb06bfa260299f4470a3e79dfa6703a617e2f2b0db3f0739b02201ee9544fd"
      "d1f7"
      "bb98e69ee3d508ea8bf5cf45d61fe4706cc8ae04b00d093668dfaf80134d21e34e47deea"
      "6bfa"
      "7956dc32678e7b85782e1e55fe79bb0214b9572285eb6473d6fa15ad3b06d10cbe7ed7a8"
      "14a9"
      "4a3be3f755a3f09720d00def0eb70bcf5c0b14fc36aaff274d78737a0e23e9782dc785b3"
      "db2a"
      "970bf7547ba446133f58327a37db11274a2d31db8e3e297474dceafd158152c95f56930a"
      "1429"
      "816b409d120e49644798bbf1dfe82d3a9ee112341b055dd39fd11c83a34d92f2ace13f96"
      "0fb2"
      "d6663f03749607056efaf67cdd43c5125b54eec0f155f2533b1806b6bda74b49a3b502ed"
      "5e40"
      "730e440109a2280c7d63cb61be3b72451910440b7fe5be14bc407fe133ee6424b76a4c91"
      "84f0"
      "5377d66d42b402f4c622b268e0f872d016eb91d796254b840fb487aad16e850d8868c0a3"
      "bc3b"
      "11a7b07165e291d7bb78c868ea961b465d4c32024508e4744e31b25e91b405d5d8a77f37"
      "1b9e"
      "feefbedbdc4d4eb674f63ade64c7b5cbad70723edb13e63d66b449a8976d299b9e14c790"
      "fef2"
      "009214555cef630cecbdc5cd12d9b10af0f43e1219851913bb3a5db443d091d8a37ceaa0"
      "77b9"
      "1b76042da33c786759bd2b17202c3aeedcdfe85d21ef0469508f7a02de66ee3ac6e5784a"
      "096d"
      "d31cb8e639a5a5887cb371288cfd07a5c1ba3a55b466ee886b300989f9bfc090e1c810e7"
      "e6b2"
      "34cf600a71259da62d8f366008935ff67006bfb38c19024cc8f4563b5c5483215cd55935"
      "0b62"
      "b06aa1964fb41a8aeb9277cd20b39654b3be2b2b116dd960e97f681e27ee1e94c640a992"
      "19b3"
      "cd8d3252913b607ce42a973e5098ab03d95fbb4b7e775e94ab6e69b3ae4ea9d0ceb2dd65"
      "a58e"
      "a2be937a3e1f9203a861000000000000e8d175abe1d580f7f96aef7e315f1b3809a5a98a"
      "919b"
      "956020e93a3b89c5352bfd601cc14e70dfcdec5b1c0ee0e7a0d100603b99d7fe082019db"
      "288a"
      "6a976416a43d3d6f9f7032b330a8dd5abbd234cb61cc067a8195865145b24a9f530f0017"
      "da25"
      "aef55da114077a8092449466945f200cc5c169066896cee56919389c4577a2173305bc72"
      "6fa1"
      "4feca7173f08e89789df2722aea3bc9fb40af352b6743356ff4d2702772925ffd065cc41"
      "b9cb"
      "6ef2359ae6bdec9753e856c321cfeb65e419aba0bd20bdd3a7f63ca8feeb63197c751d0d"
      "57e5"
      "342e67d71a1ea5ae581a69c59e1720e7731c5d0f9667bc940718290fff04e763202b7101"
      "935d"
      "2441261367e213dfbde14098f31d0deee5ac7004207d120ea3d059c0c4846910965cd35c"
      "f3a6"
      "98fca3b90f9ae159256a5f79fe7b53071da49f530b89fde2e3c67de07cf8b1ea01b93d12"
      "b26d"
      "0c456033253704f40e6f4341d2d6f157ad34144e52fbcac8f994fb761b0d0761ef731769"
      "879a"
      "9a2773749672487f6b15036e606c87ce04148e6595f8128e319eaa59774724cba8f626e6"
      "40f6"
      "be4454238b777a5750b41a07db12828d1077b7b4a0261d402233d723b31e63b253daf17f"
      "4400"
      "205ff420ffa81818462319a42762d25c6ec39838cb0b8ae3c15d7b20f96f0073a45ba986"
      "7185"
      "5372aaac9cdf0510ce38e789c340610dd1954909cb02c1118eecde817dc86641563ab2a8"
      "1be9"
      "73c0d03c4b916703b8818202076db8991df16e46b4320fa5cb594106fe22848f04a0d8ad"
      "eff4"
      "cef665543932de84bbbb15449a6c7dd10248b1322007885818a658cdcb03414ed35ec0a6"
      "08dc"
      "4e3bd1bd2dcbfcad19d96e38321c8c0f37421fe74d805acf39c8ed6e421fa49fbbc5d395"
      "45d8"
      "02be87681a5ce276eae1632928bf0769341c89ea85c8f01655b2d1c3bd824e3b9d2d060d"
      "74e0"
      "b19756a47b623b60fd6d4b1eb214a4b7536f4b3d09c10c5d942ebf7bed8b94e526780a1c"
      "432d"
      "2f148ec379a951b5e9fccbcddacc54a87b2b5c96c345b1631ece99b4af5d069dbe68b65c"
      "17f0"
      "277b028374ba9c8994b29da430488c35710f3fabe985330f07b410e2aa682e436838deb0"
      "4b64"
      "3926619db1ef6d8aa859ac634f1a508e60c1781013eb5c330d270fe65635e46c5f143a07"
      "86c3"
      "4b84de43f288d27d2eee4751df7abffe233cfdc8edd1a057eab473de22fddafca46ecbef"
      "9e03"
      "3b840abba5a6d56721d3690334adde1509f729735a32850e85fa96acc2fdcd22571c5944"
      "d8b8"
      "943018d363e86fa1dc67c0d6465767cf6705fb9661a617b40f1729fabb32fb1b79842330"
      "4fb7"
      "290568b43ed0fc5db5efe8242317b3456f53a524d6f0dad5b98936ed1a6a7fbdca1845f0"
      "70d9"
      "171f0a50b6c658ce5d1ea5989c294371f011a076616a2ab4424b140a808cb5d1cf444bb0"
      "8155"
      "e52151af419e305e0deb51344924b9218dbdf51d8d599affb19a77244f37632d270dfb51"
      "3c24"
      "50980b17290c7292e4e138d1d6098fe09bf95685381f48399135a733afdcd8a2eeb03d5b"
      "1b4d"
      "1ff3ca3a0cc9592fa5422ada207a1932c4aad777e44a01870a45903fb833373a9444f3fe"
      "0239"
      "28b91696cb86901eb00c85c81903824c1471d716cfc11ed3f4f926340af790bbc85a5eac"
      "a3c0"
      "d8d8bf1f1c89141bd59638c003c273a95638e6471057a4ba9d14267f308ae2cdffd91ec7"
      "1b99"
      "c74e9f468053a51b3f91672be5d24b616eb08cddf74b8110f32bb05c3ea56ed44d7e66dd"
      "38c3"
      "4e9d397ba797e269f455cfdcbd630ec62720c19a3c09a19e54772740c99ed9be98883ad9"
      "26c2"
      "90b321f5537da293481bd8aaae7f5ceb76f06a91214c5bb321d76637fd36f3032e73fe8d"
      "44d7"
      "16b6fea93a8d2910681a6638b7fed410c5c9bd109bde71fc74efe48b288a2a5e87fa10da"
      "7cb5"
      "c3d8c21025b6279646976897549c5a26ff013c24fe1c1395d5a936c71f86a24715764153"
      "b72d"
      "1227609ed1bb423559454d660c1f28cc62b83adab45eae6943dc952996b9aa70c1c2443b"
      "0151"
      "570611b62082cfd6874ccf452ab0803a807ef51552998c86f7e58f136774d4a46b63c202"
      "446d"
      "91cccb5fb50f0e1531839a7a8f0077c6c91ac3da14b060e97e4f4e3bb8f0a4a3a0c126e4"
      "9806"
      "a77dad2982532e093895c33422ead759133eac443647a426fd0262e2e7bf77b34dbeb065"
      "750f"
      "ae669be3b3d7af361e46cead7e107a5f975940aa328893182855a3171c57377f6f3f10e0"
      "0fc8"
      "8588b73cf06949922f7d7299801b3b281985ce321fdeab4b675237b8ecd7d0d32dd36c70"
      "8436"
      "6faa43cc9d7e0b62c3b4febcbe2facf79cc23643638fd903eeb547e5720742c9e1d351d7"
      "0c1b"
      "c2f9d576a095cb46aeca26ebcf94e5128639bc3f273b07626c329de356c1934b3ec382cc"
      "86cd"
      "19bfcaafb4aa187b9268050f706d0ccd12c27368de29db18d3c9cf68a5af9c77c24fa174"
      "b58b"
      "3d6980e99262badc809057e9999016bb8c7c806c852a00f3e8abfa7990cc2f42d82001d1"
      "0c90"
      "cc2ce822efbad7d8702cd2709ba66712b305a5536b1a3b7812dc2b9b08593fb78e878176"
      "c235"
      "d5fbada0f5bdfd60ee4c002d72a5eb946a392465eb062b6d5ba71367a44f692f789957c4"
      "bcf9"
      "a6b9e577ef640fcf48cfc00f0036943ce03942f6770329325db97994982d2df4d1b9977d"
      "c452"
      "5ff4774985a50be86708df79df4032ecf2de05719d39777632e39a1b6ebd788168f6b3d1"
      "6e07"
      "0e5e637a6e5c65fdb3e2bf553d92c4e1b9e1f31c52be1bcdf095e6c672837a8d27942745"
      "b74c"
      "99a0f78e08da56caa3a82b524f035b533b82427e229c428eda5207e7ed061822536b1d88"
      "ef5f"
      "0de24ed421a8d4cb02b5b9c5f385973f9b75b899060784ba04368895dfb101a7314104d4"
      "af2d"
      "b7589d0f3de2d4062cb50989fac23c1035cec1d9e0686ceebe84af6c756770d44ab2f7a8"
      "8d6c"
      "76b16b1a960e40212a4d22a84e4f8dd57251dfae73d67811aa4de9eb569f6feac27b6ee8"
      "e42d"
      "56982a476d71669182de65ceb69eee209a0cd962c494f24f40e2bfe89d846bb8bdbe2421"
      "1c9d"
      "9347b5a4e07a919f2aa138e7ff6c68da2a791a395f692c2875c0be082f8205ea1f7af6ea"
      "fcca"
      "40eaa19ec2ebec89dc180c1149004dd5344b987da7b511b3febdcf93cd5702d3840b5f32"
      "4026"
      "c97874b666a88cbfa10f5cec6a3c87bd3c2e6bae258aec3c42735edb708bcfd75d2b7886"
      "622f"
      "2d087f7769d454acf7a10be9dce0d831ae50e5b162b3545f7619730a8a3d8aad864e08ec"
      "6b00"
      "c60eecdb0370d084c8070ab8ea1f855da2aa893f991e1a7129870b53cc2e2f8d95f45eb6"
      "2bc1"
      "9c942523f0fb12bb2e33518931576204382c4c12bc8eca7d966a31b07ba61d208e5f9a32"
      "ff7d"
      "063056ee76c1656c9b1ea007f0d75570544db65cd68dc6591379f61348c9e29515cf8caf"
      "7207"
      "1e6a5914dca29907daf2e4671682b39353790a901d165dbb3c223d553beaf1d8f66c604f"
      "b467"
      "a4a6f7b75cbe2cdf6fec3d6107db5d868c762e77af93eaa445288afd0d67b4874783c0b3"
      "cc6d"
      "d71514c968bc239937c08cbac6bb32bb88a875940848ce0b238044dcd10d1b0e04d0ace3"
      "9330"
      "3050a58dcaf08cae9641877e1bebb621acaac9de469de4ac015c02b63773b88e3e3e8819"
      "60e6"
      "f39a6669544909900b1701386b17ade0b8af1da3dba4d1f3d1300dcd333cb4ed6118c7bb"
      "6971"
      "9ddfed6864092048fa4753f28b7dd1e38484f81e32167cf7eadef1f4cf4ada26a52f2bb1"
      "4f03"
      "046fe7b56be581872824eacf53d21df4c4df21fe591d6e3aedd1bacc4a437b8a0c93c18e"
      "3545"
      "0ec27e3f7f3a61913586ff5bf90fd59f570d8006d2b3edfa4949e177c48de61aa60fb070"
      "da91"
      "42021f364197ec2442310722f16e90e92522ecb9433c985ed92cf3284c30b7eff8b718e1"
      "c441"
      "71ebff69ccd7e1bb20f2f45d4415e12da62df3d1e5e68dce7d8906de0412a6590164cd32"
      "ff33"
      "5265f294c5b1f3ac680e29b3a0c88f4dee27d139663e2d6a7f46aa3d200c0f9bc63779b6"
      "8caa"
      "c1bd1788c81cb7c6ccf1d52413002fa60b122069c5884f2e0a2879b2f034ba6e1cc76e75"
      "8052"
      "bc9698c1b732994d36021e51dab95feb681921620bf2c24be6fcda7ef0b05acba4e94209"
      "9d0e"
      "d7ff266438e535bc1d9fbbf180d05796a0b34da0b3de4e13747c6d59737bb9479b1760c2"
      "0e8b"
      "6514dc3ee26f2204b08b53114f4aed479d3d3ace86574c9b83388b79ce562382eb99422b"
      "3001"
      "544227a5f4dbe2877463fc474e24c9d8519257294648f93132c0757fef93635770111a22"
      "21b1"
      "2a6ecc8b65ac59f255bed4579d37ad1961682366b6b53ea9007855c7aafc7d6a0e4843e3"
      "5ee9"
      "a5845d2006db7cbb5c07fe080fe3ca91ca6674df14dbf74c8362f1da4fc168644a024bc4"
      "7167"
      "5961217bc2b53ba541ec7c8cc1dae4f12c2294e10675b9e226ba63f0db0dce4d58e2b6c9"
      "1b35"
      "271786ff21291d27486dfcf6c3c6ac1231b0ededbf65808ea0675c27c2c680470d725604"
      "cf7c"
      "22dcd602e3ec889cfc9c5d97c6215000ae0e7428b39b9d685e0f0362a28226ffaad98147"
      "1378"
      "c47d338205d1ac850143970e460d74cc83551c142d763c65ab079d459e9ba093e3c8e46f"
      "156d"
      "9282068dc47ade8fdd80fbfb00932e4dfa5cf32e8d7240e9a264b581e41a3bb22ff587b1"
      "dd51"
      "80b2833b52020f531da87dc59ac672dd7a560be1caf9440886d99fe1f1f0d0fd7bd0efc8"
      "e140"
      "bbbc3ded28165c9a7a6379e637f52e2f1a82c50af2106fe2d79874b51c95de66137b0b92"
      "2ffe"
      "620eaef7dd8cd612b0ec281600a51c3efdeabc896c8141b8ecfda17d2ceae0967f7f11b0"
      "4d9b"
      "9f711f0627b68ba15b3296355bce02866b2674387d06074d3b0fd1571bfb6f32e40f842f"
      "dfa4"
      "2569ee3d2928592daced08b6dbdf64d0051d65f59308c675f8960c874420d0b10b86860d"
      "612f"
      "a4cbbabf8ef349eaaa8939f9690bfe8ac84b1b91a0624a283d905e04cf316fd08a3f2e7d"
      "a730"
      "61844fa31e1a28e0f8d7cd5d48713209db3c024ffef8aa84d6f17f947b6cba2d91bd6cb7"
      "17dd"
      "efcb05a0719559f66249ba7cb5cc102392967a8e1f1cb472f2cd8381b8e8217ce2b45be0"
      "3639"
      "206c8742c2dce60389a8dacb79871eba100b2f65981d5a1d30e03be7b2d70ce14344d4a7"
      "e4ed"
      "3c3b6300ae925bf842cdd73c6e1fd82e613d3ce4baaa4af6b119f926cbe82d0d90628c99"
      "5b6c"
      "0d53386e35839e32d8467a0b21ca1b3c438b7b700cf8fd6c31d7f3934e483f8c35b21fec"
      "66af"
      "9e3898767cb72aa22dd20794263361dad5bad1560e5c92603c418b1bef6e0c836af298c7"
      "74c3"
      "36f4e656e433686ad34e833500b2c31eaba95c6fd2542ad2e1e8d7f0034e24792a50a8c6"
      "f7c5"
      "e0237e4fb8bba5ccde22f546d1019c3b5972790554877253e41716c0abb15e6f4613b4c3"
      "87e2"
      "03a086477f8018b0fb6e2f8cac073f2d7b365e413db13608cdce817231bf97dc8de13678"
      "747c"
      "e37e5274267ff23246c84d4525962960ebd95b6ade9f17a51cd5b5037dca7a750c4ff633"
      "339a"
      "f850ac3be9c61f812beba19c903eeeefb50bcb25cb19d9a7194d50fb706f807ac653dc33"
      "3650"
      "92070a5077541bf966a1028ee24c2223150da9acde58f79f81bce9436dc411fa8eea267c"
      "1ad8"
      "5de25ebc0c0d9e8f35580b3ff533efd02636a13c4656aea6ecc5fc460b8ca3e11e08cb65"
      "691b"
      "bc5a6c041bad7ce731b168486204dbb671dc60d4c1d41e642a0682fa930afa7d3fafa665"
      "781a"
      "bab690d8e49f2422eca638f09fc7e7191931b9e2a40367d2f1397168984409dff9dda7d0"
      "f219"
      "ba21af8e69ca40b324af01b9c6a73098266111cb7e4d2ba399f6807947c74d410ebd73e8"
      "8f8e"
      "24ca683605ba0010a9448933b8ba8fc2378e9294f7d5f7dbcfd2d100bd1cbef9e62d9b32"
      "1c45"
      "7560579edbe3d15c1f06425fe20a1fb638d103778ca968106b06015ef39825bd22072a45"
      "0402"
      "604b7d6d79323bcf1d86d07ea6951dc9ed50e3d272a296570e67097996746cd6126cdfb3"
      "8f53"
      "240b03142e465a2abcc7d5d2036cb0b8fcf20a1f0b986d03ea6b837952abd4d5bc02f382"
      "4d80"
      "a2ac227fb0c33870c6b94b9f16193c469754a5edf3515df55ffef80548621131873fec5f"
      "d8a3"
      "7153845d19df2245e60868878f4f3029028b173fc8ef0cb192aafa05f74bb236616b3a20"
      "0755"
      "2e6eaffeb9d78b6d17d7a53282a8d27f5be6ca07c4501cf1d6ced9e6b85b97fc01b5e11c"
      "963f"
      "f582f0e5cd61163e8ab56e0f362fd4b33a67dc7293003671547c07089cdf07729d25a4c5"
      "2da9"
      "347a86c40303531b12d04b5a9b347c5e57fb8a1b00669061901c79be31fa96cbce6946fd"
      "b904"
      "d2de0f0ee3dc6341a24203b385f21c3009f59d940611be79e1edb45985067d34c51e8204"
      "3bf5"
      "755993e3d9db9b272020f86324385af8837445ff945902f4c6ac84cb2aaefdef3c445c54"
      "7e10"
      "9b3105ffb029132699b537a86b9ece5ff892569f20b8bc14d2aebf0abda1689bcc5815ea"
      "7a65"
      "63b0d75bc8b55b5eba0c28ee04014b506847a8ebf8e9ef4acf5e6bd014aaebbdb00bcbd8"
      "6988"
      "823957f585cc74ed2dc094931f7f72864fee711c6c0b17edf169a8e1d10c87e5c17e0f6c"
      "f53a"
      "4b2d5479eb83260f8117d102a828099429f6601f1597422d1902b0844a8ac7b1f7b792fd"
      "6087"
      "489c5f83a3e68768504e2d51c5f695205ca2887831929e12b9d95b68b483766296b1f467"
      "31cd"
      "e4617d221f88a4004a1c0d7afef7318d1d2ab41498b04bae12aad03578bbdfeaf67a5b56"
      "3d1b"
      "a3b722ef1baa34429b644cb447a4729234f50f54eaff29d0fa174f6f943639544797c3dc"
      "0693"
      "7ea9251cadf27ce335b9b6508355e6b8cd45018f1abd65fe097412df4098b137245cf13e"
      "df10"
      "40e3aee8b92395cc06e3488a3fa8554c127a387a6965056357ec4ccd8b836049d019057c"
      "35d0"
      "515c5433592b547e246dc3f832ab6bba54c1ebac56deb0814052f732e6268cf93b4e1a8b"
      "6454"
      "a69d1f87d90a13d09e292e3922d5ac8cc90fa5cb52c4316dfb23947b51c7d5539b2ef24d"
      "7b6a"
      "07470f2c2a6c76cebc483c2e5787134f8e7cf942be842c7ef3dc9338c8e007c6153d9802"
      "44b6"
      "47a5ec2e2aca79747b137a64f72621cedf36dabf475881f41ac71af11debfa82849e0927"
      "dea5"
      "6df7c31803d1ac04d4305e582a5ebc5e37d4f6f01fb642eadc60f6ffd9055de37882a858"
      "6e74"
      "90c44837f570dfc6dde7425761bf032002f43bf1f222745f2b093280a9d22ca034692e41"
      "ceb7"
      "8129081f25b35204274692c64707c9836e79dbee0dd2954ad905df116649cb0a9f9be799"
      "94cc"
      "0cd53ee2c11db3cf19d61869e9130fd6c83facb948b90e2ec0c7ca19cf7f9767da8b2871"
      "2140"
      "141938ff3d052fc54184ccc3d816877a560b6a9d79f1d065fe288f82071c510ba753861b"
      "53ce"
      "ad1473b988dff71a245b9f7f6ce8152762f72a1526880bb31c9cc9fade34fb97570a81ba"
      "67fe"
      "500adc912c3ffe5b496b3b263bd1b2eb1b8265ec73efa68fae47910426e908aac3f2ee33"
      "0c62"
      "5115033a9dc112e581dc342889d14292f75deef1081a6f92a70c07eaacf0c502a2f1d4ff"
      "8846"
      "b482811f201d3dff6435a68588d50ef0aa83ec4a84c9b9876d43286064332ba4c4830698"
      "28c7"
      "a810658212f4c5f24f02260518c66f747a80535f0e51245c125950f9993521d363fc8744"
      "1706"
      "0ac33b5b77119bc0b7292b6f704961ba634f58210a8c915830f37b3afa44f79f0c6d1372"
      "b793"
      "ab05b4272e630bd7a671e367e9826340d404228789828df4658f2195ea36d7beefd94e0c"
      "8d55"
      "51a7c59a091befd6fdc26e200a87a3f9b9b86e91397da406eb8f3a1c57932027d690ed76"
      "bff0"
      "41c866dacecb22502a17af85fa92daa5f5c467876eed109ba6beb9a342120e14666a8dbc"
      "7534"
      "1a05cc9b4d4c3aa8217da32975f985ceac2384b216dbc5d1a8bc062401c74c113b7bdba8"
      "51c5"
      "172e2d87114fb4af99ae0165e18c5f7d8d1a169989f81ee6e19d31e0643cf5c7b70d1133"
      "287a"
      "4cf116c2ca56bf7f062faaa18d567e7e71588b25295add02bcb1063b26d9f61c611aa374"
      "99e9"
      "3878d6f916f4d4489261b68af9fd187cf5829dfba39a2472ed4afe14048d1b5c66833cb2"
      "52d3"
      "04d438e757e7425f10b5617044a492c3ede3a0f50829bcf65af099f798246a1b2f3dbfd0"
      "260c"
      "1af9111602d4998ced5ca8b2fe1cc9cb387fb3859b7afec3e6aa25caae86136452e50858"
      "c4ac"
      "68d76f7b1a349c74b708a74c2f5c0c68b9c445ca5ee92fcc7c79f5f83dab38397b9de823"
      "1a5f"
      "371ad106c89e195be350a443c1f3959e1cc38f33197f1453f689dab1141e6a29bada32d6"
      "14d5"
      "56e5dd30938ead897595df7874d5faa926fbd6eb4c596c1db24f8b16ec30df65475f2629"
      "fd2f"
      "f71bd8dbe89e5a14898033c7baa88d3f36551b252d525fa6da55a9eb5d47e866773509be"
      "58d7"
      "7557639d27218dbfd595ea2ce5f09fdfc6aae1571a1b851b8fb508302f25c22dff5e85d7"
      "32e1"
      "10bc799ac21a29edb672670d27e2643c5abd81c7880554caac2b1b796f37200c19fe7dbf"
      "3963"
      "3998d47138da2bd08584abf05a7f5f59a3d4aeb59b1b6fcb24137600b6486817194d1064"
      "18df"
      "cffc086577c93d39e7e6018aae921d4abd7902a4e38400f07b7da587cae9a9046c33ed10"
      "227f"
      "855e8a4113f77ec98936f872367402896a6409129d015f0faefd3189e630d2505f3fd99d"
      "2b4b"
      "0259ca2fd37b04287ef2f117ed4360133a19225b8b366d3d604157856804bfa77f4e45f6"
      "9f20"
      "3b1eadc3ced9514e2910c3d13e32848f3e98537664a7d92b04984f20eb7c1e34349870a5"
      "9717"
      "e10b56bccd90b8453009378b94ac5f0aee37b6563c97abbb2c720ca9df10815fbe45ccd3"
      "abbe"
      "a2eecbebd6ec56d9d089f60cdb46a175d14a68d335eea61969173af517d4313513e9973b"
      "e55a"
      "932a5e37f6972da5579576f7c614146c97d88977f3e6d5f3acdb32448bd08ebe8d3ffac0"
      "e661"
      "fe07dd48b5baa4902ef1a840f38afc6df4555eb1e2bcfb75d55a3018c96579e97fdd9f85"
      "3fe9"
      "986a0a3489071657fd3c9c96becbe50be9dce6a5b060d1c7f75ca6c1a1f3ff3497043766"
      "b937"
      "b77343eaa8de10e76870717f28f61f5fb90595d96aedaf0bef974cab1ff2000c27bdcdff"
      "76ab"
      "57a4196b1fa362979b28ed4d3b1cc50e34e97d42c5500ec193760fc6483046d026b6c192"
      "5f26"
      "0ef37d06a89f2424109ea1106d8e63cc0ffd09fc50b17822e1e6f322dd43fd16cd7d446a"
      "58d5"
      "e91f8340d2d7d3c886368c3ecb7b2aff5bc3d5c7de56356266d13df5c07eea369c338e08"
      "5cfb"
      "00a4f4c291011e7d0440d602910c4824701429bec0c6b02a7b7878ec3e09b98b6fc71003"
      "b35f"
      "b96d98fd39a4861355674434b709d8a2a09ab36dcc790c86402bc87bc8269821aa059125"
      "e4ca"
      "f5dfbee1259ac604aea65d29ec0ff397bf641cbd92e6526e26f1d0771bf33128973173a5"
      "e18e"
      "91c8057852041a5efd7a88bdcb66161b562e36b695cc67dcb3adf11373eb15f0aefc44e7"
      "4352"
      "2599cbf557840f4a792b0401888ad43fde657ca9d3b11eacf356e841e3263137452b06fc"
      "5066"
      "98c07b136e93047c741ab66dd84f3879631edafd2ad60ad4ecd51366decb65de2fbf019d"
      "0b85"
      "9d16249f30db4586a19f12fc61cda91b277687776ca7ad5a0e894614749ac9303f38aef0"
      "522e"
      "415e116acee71cf15120a366c4e249dae0a12063d8f42ba22c643983150c7c220419593e"
      "e598"
      "e0b23dcb485b4ba9303275b1bdabbbd0bad1ef4197c548c6114a245742c82f8fae1b05de"
      "65be"
      "a63974d1b4426e668a8a444780170fc699822f873f3ea0f951dc04f33c6f530d2e7f43a7"
      "cb06"
      "a02404ddf47b19540f709251edd5dcfbe3b24ade5b3ee7f8c5dd6b706349229a238b8ea8"
      "2fe4"
      "3972145c2481cb5a6d3a85956ce40bbc14e01de1fab8ebe63c75bca5370f59ec96a72031"
      "6769"
      "9f883803620f403a6e039538282749333b18c712e56c2b1f01c30b3f12244fe0fc1b55fe"
      "dc46"
      "1da5840eb16867d8a687b319006fed2bcb78a20073f398eb477160613c81db44b1ccae65"
      "8aa3"
      "dcca976bdbdf813dcec47b5b3ea996eaa55a8c2c918654b1b9e628c3ca29939c443e36bd"
      "50e4"
      "06d8cd6a9056ba3058fbb51383333cf076bc1b0bc3bdbeb2dbf8612bc9921210e1550309"
      "b614"
      "f3626c47214fe667fe1e4dc3b6e5e25585e2c937d742c8d37f6fcfd5f20a833e259612d1"
      "2f91"
      "e1b0a3052be3480ae23725683bcdc08656a4cad13ea8d37372a6a4f6796a9a05f593ec0f"
      "d6a4"
      "508a32d16a4a42ca12de4526a81435c25aab9d88d301376a2eb5e56b722cb3a31492d650"
      "05a9"
      "8b651a98083da6fbed9a1882650cdf990ebd1af046da164ce51b27db2312d642b76c6529"
      "253a"
      "5d54aa36d0911acd76e03185a0e41deb634b25ab5fd7f52a0dc18f17990686dd0693af9c"
      "9e26"
      "ed821dae4b3216be9bdb7a7c31759053b3f8a78813323d5f5a59a756b2314936c5513602"
      "5a3a"
      "b825961a9d35970c916a48241e0991e8e13caec5363ca6a3d6ffe42791ceea7306ea410d"
      "c210"
      "ef1a5c20ba08b3a0e220f4fa0a4947a36fdfec2a6cfdbb3b6c3d9fe2fbb608405196d597"
      "e3e4"
      "2caf255ad4918593ed6700a920991ae14e432a5222");

  EXPECT_EQ(legacy_orchard.raw_tx->size(), 9141u);

  EXPECT_EQ(
      "0x46560590a72f3a768bc7368285d1552d71efe00a6d1e1dd7f0b23fcfbe5ed73e",
      ToHex(ZCashV6Serializer::CalculateTxIdDigest(tx)));

  EXPECT_EQ(
      "0x3ed75ebecf3fb2f0d71d1e6d0ae0ef712d55d1858236c78b763a2fa790055646",
      ToHex(ZCashV6Serializer::CalculateSignatureDigest(tx, std::nullopt)));

  auto raw_tx = ZCashV6Serializer::SerializeRawTransaction(tx);
  ASSERT_EQ(raw_tx.size(), 9200u);
  EXPECT_EQ(
      "0x0600008098b684d85b16a5378e9b3400a29b3400000110270000000000001976a91497"
      "8748a6"
      "68db7815805585c56f981a316e220b0b88ac0000029239d4614397342b8b6891f41bfb7f"
      "0268"
      "c149a15da3ee5897d2342219b8660a44ffea6f98cf2d84ea73fe9b3e0fd483812bba8545"
      "1258"
      "7df7eed0c244a47f227f6cecc81b8b9dd6e15f2d1c523ced7e1d8944373380bd2ac12c46"
      "9ea4"
      "87909a99bc68a81bd3439e6be8b39557b595a2e023338089f7cb74722e9cd4973b6a15dc"
      "ef11"
      "c452a0154a68581e256f40a6d0aa145e27213dfb98a6b3da7679a6b4176e667da8d23f99"
      "98d3"
      "851e77bf8821a99d56e1339047999521d8b94e67098f8715662e5dfc59f4ed7b08882357"
      "1d98"
      "127cc5860b37df10fd1a8971837c285e50c8e549bc7406b149ceba67ddfc344ff57ace53"
      "2a5e"
      "8b4ffcd9bfdf4e335d42d1332aac11140636b09c636ae21f83781d282010d982fe7df369"
      "d057"
      "3e43f6fb51fb434d96eb02a2c7d372a1b1344f8d56c641ae3bc5ccfb884dface9d11b60c"
      "b4f6"
      "7cdec97dd5ed26a94339a5881b37e5684876a8b6ca47d0c70164b2c8acf0f7f4d616c992"
      "cf82"
      "79cc8754b1d630b4c539d8281b291041e374cec5294e9a479454d8f14fcbfd93f801b9a3"
      "4881"
      "99de50414075799c30f701bef38451fc81470069b1045ebacee0f33ae67794d7bf1d6fdc"
      "a01b"
      "ee33f7be8f301c3cf67ce02620dbbaeb9bebf6aa5db5ed02a69e9fd33eca5d7107f52927"
      "d45a"
      "fe806b3e20b2f62b0de497ecccbf8bc4f45e96934de3ecf1047b04194ec9880b895191f2"
      "d531"
      "9a3b6830b535b2cd11cb44ef4517cd20d1ac8530acc2357cb99ae9d8be157954c5eb161e"
      "f636"
      "67662a6a3c4f7aae600a40a68806c02d951609f9079aadb374c23ed1737b1e2560b51de8"
      "69c6"
      "ea8e4d1b794930169a4ea277b4b40a6a7424554b90261cd02020426e7c5f4f1384c37653"
      "b91c"
      "724e0e20d59d5b9d7bc796747b510247fef4888e6de2e5ac7f5f746313d4caa85171a8e1"
      "247c"
      "8c3bee65b91aabb27c46d4e484441387ae4d17e2cfc4499a104269cb66643fe4e04cb5c0"
      "ba65"
      "abf4dde4f982908c05263cae198dad13258a750192d3b857c19728759765784ea2d078ca"
      "e77c"
      "f21887b86020c62a95a3a66a3458293a2286eb9dd9323c4dd5ceb80aab06c1ffd3744e86"
      "4813"
      "1c44e0c9e76c3bc2f4e64c3d0855071c91355c9c45d9b0037ca7bf62067a3fd7efad651c"
      "6e91"
      "d223dd16e6bec1c63516ee4ee897a78512515f074c564c90aebe9ec798c12ee4279d806d"
      "174b"
      "1ab5d651b681cee9c17072d61a36a0dbca595df8719b73bef9c14757a29727d38b5fe78b"
      "b20a"
      "3715bb7ad6c09c9577506a12682879f1b40459296b9aa00e06ce396b9145209b443d479b"
      "732e"
      "00dc72caca5d27bbcea557f6fd21c7869d75193ffddffebad25fcf52b177be3caf7b4d40"
      "a322"
      "0bb2dfa123996f6ec0f8d46902c02a97f2ae177d9c66711ddacb06bfa260299f4470a3e7"
      "9dfa"
      "6703a617e2f2b0db3f0739b02201ee9544fdd1f7bb98e69ee3d508ea8bf5cf45d61fe470"
      "6cc8"
      "ae04b00d093668dfaf80134d21e34e47deea6bfa7956dc32678e7b85782e1e55fe79bb02"
      "14b9"
      "572285eb6473d6fa15ad3b06d10cbe7ed7a814a94a3be3f755a3f09720d00def0eb70bcf"
      "5c0b"
      "14fc36aaff274d78737a0e23e9782dc785b3db2a970bf7547ba446133f58327a37db1127"
      "4a2d"
      "31db8e3e297474dceafd158152c95f56930a1429816b409d120e49644798bbf1dfe82d3a"
      "9ee1"
      "12341b055dd39fd11c83a34d92f2ace13f960fb2d6663f03749607056efaf67cdd43c512"
      "5b54"
      "eec0f155f2533b1806b6bda74b49a3b502ed5e40730e440109a2280c7d63cb61be3b7245"
      "1910"
      "440b7fe5be14bc407fe133ee6424b76a4c9184f05377d66d42b402f4c622b268e0f872d0"
      "16eb"
      "91d796254b840fb487aad16e850d8868c0a3bc3b11a7b07165e291d7bb78c868ea961b46"
      "5d4c"
      "32024508e4744e31b25e91b405d5d8a77f371b9efeefbedbdc4d4eb674f63ade64c7b5cb"
      "ad70"
      "723edb13e63d66b449a8976d299b9e14c790fef2009214555cef630cecbdc5cd12d9b10a"
      "f0f4"
      "3e1219851913bb3a5db443d091d8a37ceaa077b91b76042da33c786759bd2b17202c3aee"
      "dcdf"
      "e85d21ef0469508f7a02de66ee3ac6e5784a096dd31cb8e639a5a5887cb371288cfd07a5"
      "c1ba"
      "3a55b466ee886b300989f9bfc090e1c810e7e6b234cf600a71259da62d8f366008935ff6"
      "7006"
      "bfb38c19024cc8f4563b5c5483215cd559350b62b06aa1964fb41a8aeb9277cd20b39654"
      "b3be"
      "2b2b116dd960e97f681e27ee1e94c640a99219b3cd8d3252913b607ce42a973e5098ab03"
      "d95f"
      "bb4b7e775e94ab6e69b3ae4ea9d0ceb2dd65a58ea2be937a3e1f9203a861000000000000"
      "e8d1"
      "75abe1d580f7f96aef7e315f1b3809a5a98a919b956020e93a3b89c5352bfd601cc14e70"
      "dfcd"
      "ec5b1c0ee0e7a0d100603b99d7fe082019db288a6a976416a43d3d6f9f7032b330a8dd5a"
      "bbd2"
      "34cb61cc067a8195865145b24a9f530f0017da25aef55da114077a8092449466945f200c"
      "c5c1"
      "69066896cee56919389c4577a2173305bc726fa14feca7173f08e89789df2722aea3bc9f"
      "b40a"
      "f352b6743356ff4d2702772925ffd065cc41b9cb6ef2359ae6bdec9753e856c321cfeb65"
      "e419"
      "aba0bd20bdd3a7f63ca8feeb63197c751d0d57e5342e67d71a1ea5ae581a69c59e1720e7"
      "731c"
      "5d0f9667bc940718290fff04e763202b7101935d2441261367e213dfbde14098f31d0dee"
      "e5ac"
      "7004207d120ea3d059c0c4846910965cd35cf3a698fca3b90f9ae159256a5f79fe7b5307"
      "1da4"
      "9f530b89fde2e3c67de07cf8b1ea01b93d12b26d0c456033253704f40e6f4341d2d6f157"
      "ad34"
      "144e52fbcac8f994fb761b0d0761ef731769879a9a2773749672487f6b15036e606c87ce"
      "0414"
      "8e6595f8128e319eaa59774724cba8f626e640f6be4454238b777a5750b41a07db12828d"
      "1077"
      "b7b4a0261d402233d723b31e63b253daf17f4400205ff420ffa81818462319a42762d25c"
      "6ec3"
      "9838cb0b8ae3c15d7b20f96f0073a45ba98671855372aaac9cdf0510ce38e789c340610d"
      "d195"
      "4909cb02c1118eecde817dc86641563ab2a81be973c0d03c4b916703b8818202076db899"
      "1df1"
      "6e46b4320fa5cb594106fe22848f04a0d8adeff4cef665543932de84bbbb15449a6c7dd1"
      "0248"
      "b1322007885818a658cdcb03414ed35ec0a608dc4e3bd1bd2dcbfcad19d96e38321c8c0f"
      "3742"
      "1fe74d805acf39c8ed6e421fa49fbbc5d39545d802be87681a5ce276eae1632928bf0769"
      "341c"
      "89ea85c8f01655b2d1c3bd824e3b9d2d060d74e0b19756a47b623b60fd6d4b1eb214a4b7"
      "536f"
      "4b3d09c10c5d942ebf7bed8b94e526780a1c432d2f148ec379a951b5e9fccbcddacc54a8"
      "7b2b"
      "5c96c345b1631ece99b4af5d069dbe68b65c17f0277b028374ba9c8994b29da430488c35"
      "710f"
      "3fabe985330f07b410e2aa682e436838deb04b643926619db1ef6d8aa859ac634f1a508e"
      "60c1"
      "781013eb5c330d270fe65635e46c5f143a0786c34b84de43f288d27d2eee4751df7abffe"
      "233c"
      "fdc8edd1a057eab473de22fddafca46ecbef9e033b840abba5a6d56721d3690334adde15"
      "09f7"
      "29735a32850e85fa96acc2fdcd22571c5944d8b8943018d363e86fa1dc67c0d6465767cf"
      "6705"
      "fb9661a617b40f1729fabb32fb1b798423304fb7290568b43ed0fc5db5efe8242317b345"
      "6f53"
      "a524d6f0dad5b98936ed1a6a7fbdca1845f070d9171f0a50b6c658ce5d1ea5989c294371"
      "f011"
      "a076616a2ab4424b140a808cb5d1cf444bb08155e52151af419e305e0deb51344924b921"
      "8dbd"
      "f51d8d599affb19a77244f37632d270dfb513c2450980b17290c7292e4e138d1d6098fe0"
      "9bf9"
      "5685381f48399135a733afdcd8a2eeb03d5b1b4d1ff3ca3a0cc9592fa5422ada207a1932"
      "c4aa"
      "d777e44a01870a45903fb833373a9444f3fe023928b91696cb86901eb00c85c81903824c"
      "1471"
      "d716cfc11ed3f4f926340af790bbc85a5eaca3c0d8d8bf1f1c89141bd59638c003c273a9"
      "5638"
      "e6471057a4ba9d14267f308ae2cdffd91ec71b99c74e9f468053a51b3f91672be5d24b61"
      "6eb0"
      "8cddf74b8110f32bb05c3ea56ed44d7e66dd38c34e9d397ba797e269f455cfdcbd630ec6"
      "2720"
      "c19a3c09a19e54772740c99ed9be98883ad926c290b321f5537da293481bd8aaae7f5ceb"
      "76f0"
      "6a91214c5bb321d76637fd36f3032e73fe8d44d716b6fea93a8d2910681a6638b7fed410"
      "c5c9"
      "bd109bde71fc74efe48b288a2a5e87fa10da7cb5c3d8c21025b6279646976897549c5a26"
      "ff01"
      "3c24fe1c1395d5a936c71f86a24715764153b72d1227609ed1bb423559454d660c1f28cc"
      "62b8"
      "3adab45eae6943dc952996b9aa70c1c2443b0151570611b62082cfd6874ccf452ab0803a"
      "807e"
      "f51552998c86f7e58f136774d4a46b63c202446d91cccb5fb50f0e1531839a7a8f0077c6"
      "c91a"
      "c3da14b060e97e4f4e3bb8f0a4a3a0c126e49806a77dad2982532e093895c33422ead759"
      "133e"
      "ac443647a426fd0262e2e7bf77b34dbeb065750fae669be3b3d7af361e46cead7e107a5f"
      "9759"
      "40aa328893182855a3171c57377f6f3f10e00fc88588b73cf06949922f7d7299801b3b28"
      "1985"
      "ce321fdeab4b675237b8ecd7d0d32dd36c7084366faa43cc9d7e0b62c3b4febcbe2facf7"
      "9cc2"
      "3643638fd903eeb547e5720742c9e1d351d70c1bc2f9d576a095cb46aeca26ebcf94e512"
      "8639"
      "bc3f273b07626c329de356c1934b3ec382cc86cd19bfcaafb4aa187b9268050f706d0ccd"
      "12c2"
      "7368de29db18d3c9cf68a5af9c77c24fa174b58b3d6980e99262badc809057e9999016bb"
      "8c7c"
      "806c852a00f3e8abfa7990cc2f42d82001d10c90cc2ce822efbad7d8702cd2709ba66712"
      "b305"
      "a5536b1a3b7812dc2b9b08593fb78e878176c235d5fbada0f5bdfd60ee4c002d72a5eb94"
      "6a39"
      "2465eb062b6d5ba71367a44f692f789957c4bcf9a6b9e577ef640fcf48cfc00f0036943c"
      "e039"
      "42f6770329325db97994982d2df4d1b9977dc4525ff4774985a50be86708df79df4032ec"
      "f2de"
      "05719d39777632e39a1b6ebd788168f6b3d16e070e5e637a6e5c65fdb3e2bf553d92c4e1"
      "b9e1"
      "f31c52be1bcdf095e6c672837a8d27942745b74c99a0f78e08da56caa3a82b524f035b53"
      "3b82"
      "427e229c428eda5207e7ed061822536b1d88ef5f0de24ed421a8d4cb02b5b9c5f385973f"
      "9b75"
      "b899060784ba04368895dfb101a7314104d4af2db7589d0f3de2d4062cb50989fac23c10"
      "35ce"
      "c1d9e0686ceebe84af6c756770d44ab2f7a88d6c76b16b1a960e40212a4d22a84e4f8dd5"
      "7251"
      "dfae73d67811aa4de9eb569f6feac27b6ee8e42d56982a476d71669182de65ceb69eee20"
      "9a0c"
      "d962c494f24f40e2bfe89d846bb8bdbe24211c9d9347b5a4e07a919f2aa138e7ff6c68da"
      "2a79"
      "1a395f692c2875c0be082f8205ea1f7af6eafcca40eaa19ec2ebec89dc180c1149004dd5"
      "344b"
      "987da7b511b3febdcf93cd5702d3840b5f324026c97874b666a88cbfa10f5cec6a3c87bd"
      "3c2e"
      "6bae258aec3c42735edb708bcfd75d2b7886622f2d087f7769d454acf7a10be9dce0d831"
      "ae50"
      "e5b162b3545f7619730a8a3d8aad864e08ec6b00c60eecdb0370d084c8070ab8ea1f855d"
      "a2aa"
      "893f991e1a7129870b53cc2e2f8d95f45eb62bc19c942523f0fb12bb2e33518931576204"
      "382c"
      "4c12bc8eca7d966a31b07ba61d208e5f9a32ff7d063056ee76c1656c9b1ea007f0d75570"
      "544d"
      "b65cd68dc6591379f61348c9e29515cf8caf72071e6a5914dca29907daf2e4671682b393"
      "5379"
      "0a901d165dbb3c223d553beaf1d8f66c604fb467a4a6f7b75cbe2cdf6fec3d6107db5d86"
      "8c76"
      "2e77af93eaa445288afd0d67b4874783c0b3cc6dd71514c968bc239937c08cbac6bb32bb"
      "88a8"
      "75940848ce0b238044dcd10d1b0e04d0ace393303050a58dcaf08cae9641877e1bebb621"
      "acaa"
      "c9de469de4ac015c02b63773b88e3e3e881960e6f39a6669544909900b1701386b17ade0"
      "b8af"
      "1da3dba4d1f3d1300dcd333cb4ed6118c7bb69719ddfed6864092048fa4753f28b7dd1e3"
      "8484"
      "f81e32167cf7eadef1f4cf4ada26a52f2bb14f03046fe7b56be581872824eacf53d21df4"
      "c4df"
      "21fe591d6e3aedd1bacc4a437b8a0c93c18e35450ec27e3f7f3a61913586ff5bf90fd59f"
      "570d"
      "8006d2b3edfa4949e177c48de61aa60fb070da9142021f364197ec2442310722f16e90e9"
      "2522"
      "ecb9433c985ed92cf3284c30b7eff8b718e1c44171ebff69ccd7e1bb20f2f45d4415e12d"
      "a62d"
      "f3d1e5e68dce7d8906de0412a6590164cd32ff335265f294c5b1f3ac680e29b3a0c88f4d"
      "ee27"
      "d139663e2d6a7f46aa3d200c0f9bc63779b68caac1bd1788c81cb7c6ccf1d52413002fa6"
      "0b12"
      "2069c5884f2e0a2879b2f034ba6e1cc76e758052bc9698c1b732994d36021e51dab95feb"
      "6819"
      "21620bf2c24be6fcda7ef0b05acba4e942099d0ed7ff266438e535bc1d9fbbf180d05796"
      "a0b3"
      "4da0b3de4e13747c6d59737bb9479b1760c20e8b6514dc3ee26f2204b08b53114f4aed47"
      "9d3d"
      "3ace86574c9b83388b79ce562382eb99422b3001544227a5f4dbe2877463fc474e24c9d8"
      "5192"
      "57294648f93132c0757fef93635770111a2221b12a6ecc8b65ac59f255bed4579d37ad19"
      "6168"
      "2366b6b53ea9007855c7aafc7d6a0e4843e35ee9a5845d2006db7cbb5c07fe080fe3ca91"
      "ca66"
      "74df14dbf74c8362f1da4fc168644a024bc471675961217bc2b53ba541ec7c8cc1dae4f1"
      "2c22"
      "94e10675b9e226ba63f0db0dce4d58e2b6c91b35271786ff21291d27486dfcf6c3c6ac12"
      "31b0"
      "ededbf65808ea0675c27c2c680470d725604cf7c22dcd602e3ec889cfc9c5d97c6215000"
      "ae0e"
      "7428b39b9d685e0f0362a28226ffaad981471378c47d338205d1ac850143970e460d74cc"
      "8355"
      "1c142d763c65ab079d459e9ba093e3c8e46f156d9282068dc47ade8fdd80fbfb00932e4d"
      "fa5c"
      "f32e8d7240e9a264b581e41a3bb22ff587b1dd5180b2833b52020f531da87dc59ac672dd"
      "7a56"
      "0be1caf9440886d99fe1f1f0d0fd7bd0efc8e140bbbc3ded28165c9a7a6379e637f52e2f"
      "1a82"
      "c50af2106fe2d79874b51c95de66137b0b922ffe620eaef7dd8cd612b0ec281600a51c3e"
      "fdea"
      "bc896c8141b8ecfda17d2ceae0967f7f11b04d9b9f711f0627b68ba15b3296355bce0286"
      "6b26"
      "74387d06074d3b0fd1571bfb6f32e40f842fdfa42569ee3d2928592daced08b6dbdf64d0"
      "051d"
      "65f59308c675f8960c874420d0b10b86860d612fa4cbbabf8ef349eaaa8939f9690bfe8a"
      "c84b"
      "1b91a0624a283d905e04cf316fd08a3f2e7da73061844fa31e1a28e0f8d7cd5d48713209"
      "db3c"
      "024ffef8aa84d6f17f947b6cba2d91bd6cb717ddefcb05a0719559f66249ba7cb5cc1023"
      "9296"
      "7a8e1f1cb472f2cd8381b8e8217ce2b45be03639206c8742c2dce60389a8dacb79871eba"
      "100b"
      "2f65981d5a1d30e03be7b2d70ce14344d4a7e4ed3c3b6300ae925bf842cdd73c6e1fd82e"
      "613d"
      "3ce4baaa4af6b119f926cbe82d0d90628c995b6c0d53386e35839e32d8467a0b21ca1b3c"
      "438b"
      "7b700cf8fd6c31d7f3934e483f8c35b21fec66af9e3898767cb72aa22dd20794263361da"
      "d5ba"
      "d1560e5c92603c418b1bef6e0c836af298c774c336f4e656e433686ad34e833500b2c31e"
      "aba9"
      "5c6fd2542ad2e1e8d7f0034e24792a50a8c6f7c5e0237e4fb8bba5ccde22f546d1019c3b"
      "5972"
      "790554877253e41716c0abb15e6f4613b4c387e203a086477f8018b0fb6e2f8cac073f2d"
      "7b36"
      "5e413db13608cdce817231bf97dc8de13678747ce37e5274267ff23246c84d4525962960"
      "ebd9"
      "5b6ade9f17a51cd5b5037dca7a750c4ff633339af850ac3be9c61f812beba19c903eeeef"
      "b50b"
      "cb25cb19d9a7194d50fb706f807ac653dc33365092070a5077541bf966a1028ee24c2223"
      "150d"
      "a9acde58f79f81bce9436dc411fa8eea267c1ad85de25ebc0c0d9e8f35580b3ff533efd0"
      "2636"
      "a13c4656aea6ecc5fc460b8ca3e11e08cb65691bbc5a6c041bad7ce731b168486204dbb6"
      "71dc"
      "60d4c1d41e642a0682fa930afa7d3fafa665781abab690d8e49f2422eca638f09fc7e719"
      "1931"
      "b9e2a40367d2f1397168984409dff9dda7d0f219ba21af8e69ca40b324af01b9c6a73098"
      "2661"
      "11cb7e4d2ba399f6807947c74d410ebd73e88f8e24ca683605ba0010a9448933b8ba8fc2"
      "378e"
      "9294f7d5f7dbcfd2d100bd1cbef9e62d9b321c457560579edbe3d15c1f06425fe20a1fb6"
      "38d1"
      "03778ca968106b06015ef39825bd22072a450402604b7d6d79323bcf1d86d07ea6951dc9"
      "ed50"
      "e3d272a296570e67097996746cd6126cdfb38f53240b03142e465a2abcc7d5d2036cb0b8"
      "fcf2"
      "0a1f0b986d03ea6b837952abd4d5bc02f3824d80a2ac227fb0c33870c6b94b9f16193c46"
      "9754"
      "a5edf3515df55ffef80548621131873fec5fd8a37153845d19df2245e60868878f4f3029"
      "028b"
      "173fc8ef0cb192aafa05f74bb236616b3a2007552e6eaffeb9d78b6d17d7a53282a8d27f"
      "5be6"
      "ca07c4501cf1d6ced9e6b85b97fc01b5e11c963ff582f0e5cd61163e8ab56e0f362fd4b3"
      "3a67"
      "dc7293003671547c07089cdf07729d25a4c52da9347a86c40303531b12d04b5a9b347c5e"
      "57fb"
      "8a1b00669061901c79be31fa96cbce6946fdb904d2de0f0ee3dc6341a24203b385f21c30"
      "09f5"
      "9d940611be79e1edb45985067d34c51e82043bf5755993e3d9db9b272020f86324385af8"
      "8374"
      "45ff945902f4c6ac84cb2aaefdef3c445c547e109b3105ffb029132699b537a86b9ece5f"
      "f892"
      "569f20b8bc14d2aebf0abda1689bcc5815ea7a6563b0d75bc8b55b5eba0c28ee04014b50"
      "6847"
      "a8ebf8e9ef4acf5e6bd014aaebbdb00bcbd86988823957f585cc74ed2dc094931f7f7286"
      "4fee"
      "711c6c0b17edf169a8e1d10c87e5c17e0f6cf53a4b2d5479eb83260f8117d102a8280994"
      "29f6"
      "601f1597422d1902b0844a8ac7b1f7b792fd6087489c5f83a3e68768504e2d51c5f69520"
      "5ca2"
      "887831929e12b9d95b68b483766296b1f46731cde4617d221f88a4004a1c0d7afef7318d"
      "1d2a"
      "b41498b04bae12aad03578bbdfeaf67a5b563d1ba3b722ef1baa34429b644cb447a47292"
      "34f5"
      "0f54eaff29d0fa174f6f943639544797c3dc06937ea9251cadf27ce335b9b6508355e6b8"
      "cd45"
      "018f1abd65fe097412df4098b137245cf13edf1040e3aee8b92395cc06e3488a3fa8554c"
      "127a"
      "387a6965056357ec4ccd8b836049d019057c35d0515c5433592b547e246dc3f832ab6bba"
      "54c1"
      "ebac56deb0814052f732e6268cf93b4e1a8b6454a69d1f87d90a13d09e292e3922d5ac8c"
      "c90f"
      "a5cb52c4316dfb23947b51c7d5539b2ef24d7b6a07470f2c2a6c76cebc483c2e5787134f"
      "8e7c"
      "f942be842c7ef3dc9338c8e007c6153d980244b647a5ec2e2aca79747b137a64f72621ce"
      "df36"
      "dabf475881f41ac71af11debfa82849e0927dea56df7c31803d1ac04d4305e582a5ebc5e"
      "37d4"
      "f6f01fb642eadc60f6ffd9055de37882a8586e7490c44837f570dfc6dde7425761bf0320"
      "02f4"
      "3bf1f222745f2b093280a9d22ca034692e41ceb78129081f25b35204274692c64707c983"
      "6e79"
      "dbee0dd2954ad905df116649cb0a9f9be79994cc0cd53ee2c11db3cf19d61869e9130fd6"
      "c83f"
      "acb948b90e2ec0c7ca19cf7f9767da8b28712140141938ff3d052fc54184ccc3d816877a"
      "560b"
      "6a9d79f1d065fe288f82071c510ba753861b53cead1473b988dff71a245b9f7f6ce81527"
      "62f7"
      "2a1526880bb31c9cc9fade34fb97570a81ba67fe500adc912c3ffe5b496b3b263bd1b2eb"
      "1b82"
      "65ec73efa68fae47910426e908aac3f2ee330c625115033a9dc112e581dc342889d14292"
      "f75d"
      "eef1081a6f92a70c07eaacf0c502a2f1d4ff8846b482811f201d3dff6435a68588d50ef0"
      "aa83"
      "ec4a84c9b9876d43286064332ba4c483069828c7a810658212f4c5f24f02260518c66f74"
      "7a80"
      "535f0e51245c125950f9993521d363fc874417060ac33b5b77119bc0b7292b6f704961ba"
      "634f"
      "58210a8c915830f37b3afa44f79f0c6d1372b793ab05b4272e630bd7a671e367e9826340"
      "d404"
      "228789828df4658f2195ea36d7beefd94e0c8d5551a7c59a091befd6fdc26e200a87a3f9"
      "b9b8"
      "6e91397da406eb8f3a1c57932027d690ed76bff041c866dacecb22502a17af85fa92daa5"
      "f5c4"
      "67876eed109ba6beb9a342120e14666a8dbc75341a05cc9b4d4c3aa8217da32975f985ce"
      "ac23"
      "84b216dbc5d1a8bc062401c74c113b7bdba851c5172e2d87114fb4af99ae0165e18c5f7d"
      "8d1a"
      "169989f81ee6e19d31e0643cf5c7b70d1133287a4cf116c2ca56bf7f062faaa18d567e7e"
      "7158"
      "8b25295add02bcb1063b26d9f61c611aa37499e93878d6f916f4d4489261b68af9fd187c"
      "f582"
      "9dfba39a2472ed4afe14048d1b5c66833cb252d304d438e757e7425f10b5617044a492c3"
      "ede3"
      "a0f50829bcf65af099f798246a1b2f3dbfd0260c1af9111602d4998ced5ca8b2fe1cc9cb"
      "387f"
      "b3859b7afec3e6aa25caae86136452e50858c4ac68d76f7b1a349c74b708a74c2f5c0c68"
      "b9c4"
      "45ca5ee92fcc7c79f5f83dab38397b9de8231a5f371ad106c89e195be350a443c1f3959e"
      "1cc3"
      "8f33197f1453f689dab1141e6a29bada32d614d556e5dd30938ead897595df7874d5faa9"
      "26fb"
      "d6eb4c596c1db24f8b16ec30df65475f2629fd2ff71bd8dbe89e5a14898033c7baa88d3f"
      "3655"
      "1b252d525fa6da55a9eb5d47e866773509be58d77557639d27218dbfd595ea2ce5f09fdf"
      "c6aa"
      "e1571a1b851b8fb508302f25c22dff5e85d732e110bc799ac21a29edb672670d27e2643c"
      "5abd"
      "81c7880554caac2b1b796f37200c19fe7dbf39633998d47138da2bd08584abf05a7f5f59"
      "a3d4"
      "aeb59b1b6fcb24137600b6486817194d106418dfcffc086577c93d39e7e6018aae921d4a"
      "bd79"
      "02a4e38400f07b7da587cae9a9046c33ed10227f855e8a4113f77ec98936f87236740289"
      "6a64"
      "09129d015f0faefd3189e630d2505f3fd99d2b4b0259ca2fd37b04287ef2f117ed436013"
      "3a19"
      "225b8b366d3d604157856804bfa77f4e45f69f203b1eadc3ced9514e2910c3d13e32848f"
      "3e98"
      "537664a7d92b04984f20eb7c1e34349870a59717e10b56bccd90b8453009378b94ac5f0a"
      "ee37"
      "b6563c97abbb2c720ca9df10815fbe45ccd3abbea2eecbebd6ec56d9d089f60cdb46a175"
      "d14a"
      "68d335eea61969173af517d4313513e9973be55a932a5e37f6972da5579576f7c614146c"
      "97d8"
      "8977f3e6d5f3acdb32448bd08ebe8d3ffac0e661fe07dd48b5baa4902ef1a840f38afc6d"
      "f455"
      "5eb1e2bcfb75d55a3018c96579e97fdd9f853fe9986a0a3489071657fd3c9c96becbe50b"
      "e9dc"
      "e6a5b060d1c7f75ca6c1a1f3ff3497043766b937b77343eaa8de10e76870717f28f61f5f"
      "b905"
      "95d96aedaf0bef974cab1ff2000c27bdcdff76ab57a4196b1fa362979b28ed4d3b1cc50e"
      "34e9"
      "7d42c5500ec193760fc6483046d026b6c1925f260ef37d06a89f2424109ea1106d8e63cc"
      "0ffd"
      "09fc50b17822e1e6f322dd43fd16cd7d446a58d5e91f8340d2d7d3c886368c3ecb7b2aff"
      "5bc3"
      "d5c7de56356266d13df5c07eea369c338e085cfb00a4f4c291011e7d0440d602910c4824"
      "7014"
      "29bec0c6b02a7b7878ec3e09b98b6fc71003b35fb96d98fd39a4861355674434b709d8a2"
      "a09a"
      "b36dcc790c86402bc87bc8269821aa059125e4caf5dfbee1259ac604aea65d29ec0ff397"
      "bf64"
      "1cbd92e6526e26f1d0771bf33128973173a5e18e91c8057852041a5efd7a88bdcb66161b"
      "562e"
      "36b695cc67dcb3adf11373eb15f0aefc44e743522599cbf557840f4a792b0401888ad43f"
      "de65"
      "7ca9d3b11eacf356e841e3263137452b06fc506698c07b136e93047c741ab66dd84f3879"
      "631e"
      "dafd2ad60ad4ecd51366decb65de2fbf019d0b859d16249f30db4586a19f12fc61cda91b"
      "2776"
      "87776ca7ad5a0e894614749ac9303f38aef0522e415e116acee71cf15120a366c4e249da"
      "e0a1"
      "2063d8f42ba22c643983150c7c220419593ee598e0b23dcb485b4ba9303275b1bdabbbd0"
      "bad1"
      "ef4197c548c6114a245742c82f8fae1b05de65bea63974d1b4426e668a8a444780170fc6"
      "9982"
      "2f873f3ea0f951dc04f33c6f530d2e7f43a7cb06a02404ddf47b19540f709251edd5dcfb"
      "e3b2"
      "4ade5b3ee7f8c5dd6b706349229a238b8ea82fe43972145c2481cb5a6d3a85956ce40bbc"
      "14e0"
      "1de1fab8ebe63c75bca5370f59ec96a7203167699f883803620f403a6e03953828274933"
      "3b18"
      "c712e56c2b1f01c30b3f12244fe0fc1b55fedc461da5840eb16867d8a687b319006fed2b"
      "cb78"
      "a20073f398eb477160613c81db44b1ccae658aa3dcca976bdbdf813dcec47b5b3ea996ea"
      "a55a"
      "8c2c918654b1b9e628c3ca29939c443e36bd50e406d8cd6a9056ba3058fbb51383333cf0"
      "76bc"
      "1b0bc3bdbeb2dbf8612bc9921210e1550309b614f3626c47214fe667fe1e4dc3b6e5e255"
      "85e2"
      "c937d742c8d37f6fcfd5f20a833e259612d12f91e1b0a3052be3480ae23725683bcdc086"
      "56a4"
      "cad13ea8d37372a6a4f6796a9a05f593ec0fd6a4508a32d16a4a42ca12de4526a81435c2"
      "5aab"
      "9d88d301376a2eb5e56b722cb3a31492d65005a98b651a98083da6fbed9a1882650cdf99"
      "0ebd"
      "1af046da164ce51b27db2312d642b76c6529253a5d54aa36d0911acd76e03185a0e41deb"
      "634b"
      "25ab5fd7f52a0dc18f17990686dd0693af9c9e26ed821dae4b3216be9bdb7a7c31759053"
      "b3f8"
      "a78813323d5f5a59a756b2314936c55136025a3ab825961a9d35970c916a48241e0991e8"
      "e13c"
      "aec5363ca6a3d6ffe42791ceea7306ea410dc210ef1a5c20ba08b3a0e220f4fa0a4947a3"
      "6fdf"
      "ec2a6cfdbb3b6c3d9fe2fbb608405196d597e3e42caf255ad4918593ed6700a920991ae1"
      "4e43"
      "2a522200",
      ToHex(raw_tx));
}

}  // namespace brave_wallet
