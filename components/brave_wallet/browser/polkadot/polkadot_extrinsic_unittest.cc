/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48";

// These can be trivially derived using something like:
// clang-format off
//
// curl -H "Content-Type: application/json" -d '{"id":1, "jsonrpc":"2.0", "method": "system_chain", "params":[]}' https://westend-rpc.polkadot.io
// outputs => {"jsonrpc":"2.0","id":1,"result":"Westend"}
//
// clang-format on
inline constexpr const char kWestendChainType[] = "Westend";
inline constexpr const char kPolkadotChainType[] = "Polkadot";
inline constexpr const char kWestendAssetHubChainType[] = "Westend Asset Hub";
inline constexpr const char kPolkadotAssetHubChainType[] = "Polkadot Asset Hub";

// Taken from:
// https://docs.rs/schnorrkel/0.11.4/schnorrkel/keys/struct.MiniSecretKey.html#method.from_bytes
constexpr uint8_t kSchnorrkelSeed[] = {
    157, 97,  177, 157, 239, 253, 90, 96,  186, 132, 74,
    244, 146, 236, 44,  196, 68,  73, 197, 105, 123, 50,
    105, 25,  112, 59,  172, 3,   28, 174, 127, 96,
};

}  // namespace

TEST(PolkadotExtrinsics, MetadataFromChainName) {
  EXPECT_TRUE(PolkadotChainMetadata::FromChainName(kWestendChainType));
  EXPECT_TRUE(PolkadotChainMetadata::FromChainName(kPolkadotChainType));
  EXPECT_TRUE(PolkadotChainMetadata::FromChainName(kWestendAssetHubChainType));
  EXPECT_TRUE(PolkadotChainMetadata::FromChainName(kPolkadotAssetHubChainType));

  EXPECT_FALSE(PolkadotChainMetadata::FromChainName("random text"));
  EXPECT_FALSE(PolkadotChainMetadata::FromChainName(""));
}

TEST(PolkadotExtrinsics, UnsignedTransfer) {
  // Test we can construct an unsigned extrinsic representing a
  // transfer_allow_death call and then serialize it appropriately to a hex
  // string.

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  auto mainnet_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotChainType).value();

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  uint128_t send_amount = 1234;

  PolkadotUnsignedTransfer transfer_extrinsic(pubkey, send_amount);

  // This test vector matches the output from the polkadot-js api:
  // clang-format off
  //
  //   import { ApiPromise, HttpProvider } from '@polkadot/api';
  //
  //   const httpProvider = new HttpProvider('https://westend-rpc.polkadot.io');
  //   const api = await ApiPromise.create({ provider: httpProvider });
  //   const BOB = '5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty';
  //   const transfer = api.tx.balances.transferAllowDeath(BOB, 1234);
  //   console.log(transfer.Encode());
  //
  //   outputs 0x98040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913
  // clang-format on

  const char* testnet_extrinsic =
      R"(98040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  EXPECT_EQ(transfer_extrinsic.send_amount(), 1234u);
  EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.recipient()), kBob);
  EXPECT_EQ(transfer_extrinsic.Encode(testnet_metadata), testnet_extrinsic);

  // The above example JS uses a Polkadot testnet. For mainnet, we should have a
  // different index for the Balances pallet.
  // We build the API off of the RPC endpoints here:
  //   const httpProvider = new HttpProvider('https://dot-rpc.stakeworld.io');
  //
  // A full list of Polkadot network hosts can be found here:
  // https://docs.polkadot.com/develop/networks
  //
  // There are other available mainnets we can test with such as:
  // https://polkadot-public-rpc.blockops.network/rpc

  const char* mainnet_extrinsic =
      R"(98040500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  EXPECT_EQ(transfer_extrinsic.Encode(mainnet_metadata), mainnet_extrinsic);
}

TEST(PolkadotExtrinsics, UnsignedTransferAssetHub) {
  // Test that when our code is setup to point to a different parachain, such as
  // AssetHub, that we can still generate an extrinsic with the correct pallet
  // and call indices.

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendAssetHubChainType).value();

  auto mainnet_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotAssetHubChainType).value();

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  uint128_t send_amount = 1234;

  PolkadotUnsignedTransfer transfer_extrinsic(pubkey, send_amount);

  // This test vector matches the output from the polkadot-js api:
  // clang-format off
  //
  //   import { ApiPromise, HttpProvider } from '@polkadot/api';
  //
  //   const httpProvider = new HttpProvider('https://westend-asset-hub-rpc.polkadot.io');
  //   const api = await ApiPromise.create({ provider: httpProvider });
  //   const BOB = '5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty';
  //   const transfer = api.tx.balances.transferAllowDeath(BOB, 1234);
  //   console.log(transfer.toHex());
  //
  //   outputs 0x98040a00008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913
  // clang-format on

  const char* testnet_extrinsic =
      R"(98040a00008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  EXPECT_EQ(transfer_extrinsic.send_amount(), 1234u);
  EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.recipient()), kBob);
  EXPECT_EQ(transfer_extrinsic.Encode(testnet_metadata), testnet_extrinsic);

  // The above example JS uses a Polkadot testnet. For mainnet, we should have a
  // different index for the Balances pallet.
  // We build the API off of the RPC endpoints here:
  // clang-format off
  //
  //   const httpProvider = new HttpProvider('https://asset-hub-polkadot-rpc.n.dwellir.com');
  //
  // clang-format on
  //
  // A full list of Polkadot network hosts can be found here:
  // https://docs.polkadot.com/develop/networks
  //
  // There are other available mainnets we can test with such as:
  // https://polkadot-public-rpc.blockops.network/rpc

  const char* mainnet_extrinsic =
      R"(98040a00008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  EXPECT_EQ(transfer_extrinsic.Encode(mainnet_metadata), mainnet_extrinsic);
}

TEST(PolkadotExtrinsics, UnsignedTransferNumericLimits) {
  // Test our extrinsic creation and serialization using numeric limits for a
  // uint128_t.

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  {
    // Extrinsics like these are doomed to fail, and they roughly wind up
    // looking like these:
    // https://assethub-westend.subscan.io/extrinsic/13197653-2
    // https://assethub-westend.subscan.io/extrinsic/13197730-2
    //
    // We may be able to prevent the user from over-spending on the UI side but
    // the RPC nodes will happily process these transactions regardless, then we
    // must examine the events for a given block to determine the status of the
    // extrinsic, probing for the "ExtrinsicFailed".

    uint128_t send_amount = std::numeric_limits<uint128_t>::max();
    PolkadotUnsignedTransfer transfer_extrinsic(pubkey, send_amount);

    const char* testnet_extrinsic =
        R"(d4040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4833ffffffffffffffffffffffffffffffff)";
    EXPECT_EQ(transfer_extrinsic.Encode(testnet_metadata), testnet_extrinsic);
  }

  {
    uint128_t send_amount = 0;
    PolkadotUnsignedTransfer transfer_extrinsic(pubkey, send_amount);

    const char* testnet_extrinsic =
        R"(94040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4800)";
    EXPECT_EQ(transfer_extrinsic.Encode(testnet_metadata), testnet_extrinsic);
  }
}

TEST(PolkadotExtrinsics, DecodedUnsignedTransfer) {
  // Test that we can appropriately decode the hex representation of an
  // extrinsic for a given relay chain.

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  auto mainnet_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotChainType).value();

  // These extrinsics can be verified using the polkadot-js API as such:
  // clang-format off
  //
  //   import { GenericExtrinsic as Extrinsic } from '@polkadot/types/extrinsic';
  //   import { GENERAL_EXTRINSIC } from '@polkadot/types/extrinsic/constants';
  //   import { ApiPromise, HttpProvider } from '@polkadot/api';
  //
  //   const httpProvider = new HttpProvider('https://westend-rpc.polkadot.io');
  //   const api = await ApiPromise.create({ provider: httpProvider });
  //   const tester = new Extrinsic(api.registry, '0x98040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913', { preamble: GENERAL_EXTRINSIC, version: 4});
  //   console.log(JSON.stringify(tester.toHuman()));
  //
  //   outputs => {"isSigned":false,"method":{"args":{"dest":{"Id":"5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty"},"value":"1,234"},"method":"transferAllowDeath","section":"balances"}}
  // clang-format on

  {
    const char* testnet_extrinsic =
        R"(98040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

    auto transfer_extrinsic =
        PolkadotUnsignedTransfer::Decode(testnet_metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()),
              kBob);
  }

  {
    const char* mainnet_extrinsic =
        R"(98040500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

    auto transfer_extrinsic =
        PolkadotUnsignedTransfer::Decode(mainnet_metadata, mainnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()),
              kBob);
  }
}

TEST(PolkadotExtrinsics, DecodedUnsignedTransferAssetHub) {
  // Test that we can decode extrinsics for a specific parachain, like AssetHub.

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendAssetHubChainType).value();

  auto mainnet_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotAssetHubChainType).value();

  // These extrinsics can be verified using the polkadot-js API as such:
  // clang-format off
  //
  //   import { GenericExtrinsic as Extrinsic } from '@polkadot/types/extrinsic';
  //   import { GENERAL_EXTRINSIC } from '@polkadot/types/extrinsic/constants';
  //   import { ApiPromise, HttpProvider } from '@polkadot/api';
  //
  //   const httpProvider = new HttpProvider('https://westend-asset-hub-rpc.polkadot.io');
  //   const api = await ApiPromise.create({ provider: httpProvider });
  //   const tester = new Extrinsic(api.registry, '0x98040A00008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913', { preamble: GENERAL_EXTRINSIC, version: 4});
  //   console.log(JSON.stringify(tester.toHuman()));
  //
  //   outputs => {"isSigned":false,"method":{"args":{"dest":{"Id":"5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty"},"value":"1,234"},"method":"transferAllowDeath","section":"balances"}}
  // clang-format on

  {
    const char* testnet_extrinsic =
        R"(98040a00008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

    auto transfer_extrinsic =
        PolkadotUnsignedTransfer::Decode(testnet_metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()),
              kBob);
  }

  {
    const char* mainnet_extrinsic =
        R"(98040a00008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

    auto transfer_extrinsic =
        PolkadotUnsignedTransfer::Decode(mainnet_metadata, mainnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()),
              kBob);
  }
}

TEST(PolkadotExtrinsics, DecodeNumericLimits) {
  // Test extrinsic decoding for numeric limits.

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  {
    const char* testnet_extrinsic =
        R"(d4040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4833ffffffffffffffffffffffffffffffff)";

    auto transfer_extrinsic =
        PolkadotUnsignedTransfer::Decode(testnet_metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(),
              std::numeric_limits<uint128_t>::max());
    EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()),
              kBob);
  }

  {
    const char testnet_extrinsic[] =
        R"(94040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4800)";

    auto transfer_extrinsic =
        PolkadotUnsignedTransfer::Decode(testnet_metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 0u);
    EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()),
              kBob);
  }
}

TEST(PolkadotExtrinsics, InvalidDecode) {
  // Test that subtle differences in the hex-encoded extrinsics will cause our
  // code to fail to parse.

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  auto mainnet_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotChainType).value();

  {
    // Valid data, but not enough.

    std::string_view mainnet_extrinsic =
        R"(98040500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

    for (size_t i = 0; i < mainnet_extrinsic.size() - 1; ++i) {
      auto input = mainnet_extrinsic.substr(0, i);
      auto transfer_extrinsic =
          PolkadotUnsignedTransfer::Decode(mainnet_metadata, input);
      EXPECT_FALSE(transfer_extrinsic) << input;
    }
  }

  {
    std::string_view inputs[] = {
        R"(55040500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)",  // Invalid leading length.
        R"(98110500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)",  // Invalid extrinsic version.
        R"(98041200008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)",  // Invalid pallet index.
        R"(98040534008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)",  // Invalid call index.
        R"(98040500018eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)",  // Invalid MultiAddress type.
        R"(98040500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a481234)",  // Invalid send amount.
        R"(98040500008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4833ffffffffffffffffffffffffffffffffffffff)",  // Send amount would exceed numeric limits.
    };

    for (auto input : inputs) {
      auto transfer_extrinsic =
          PolkadotUnsignedTransfer::Decode(mainnet_metadata, input);
      EXPECT_FALSE(transfer_extrinsic) << input;
    }
  }
}

TEST(PolkadotExtrinsics, InvalidDecodeFromIncompatibleParachain) {
  // Test the case where we have a valid extrinsics for a specific parachain
  // that's not compatible with the current one we're supplying.

  const char* testnet_extrinsic =
      R"(98040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  auto mainnet_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotChainType).value();

  auto testnet_assethub_metadata =
      PolkadotChainMetadata::FromChainName(kWestendAssetHubChainType).value();

  auto mainnet_assethub_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotAssetHubChainType).value();

  auto transfer_extrinsic =
      PolkadotUnsignedTransfer::Decode(testnet_metadata, testnet_extrinsic);

  EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
  EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()), kBob);

  EXPECT_FALSE(
      PolkadotUnsignedTransfer::Decode(mainnet_metadata, testnet_extrinsic));
  EXPECT_FALSE(PolkadotUnsignedTransfer::Decode(testnet_assethub_metadata,
                                                testnet_extrinsic));
  EXPECT_FALSE(PolkadotUnsignedTransfer::Decode(mainnet_assethub_metadata,
                                                testnet_extrinsic));
}

TEST(PolkadotExtrinsics, MortalityEncoding) {
  // clang-format off
  /*
    Test vectors were generated using the polkadot-sdk:

    use parity_scale_codec::Encode;
    use polkadot_sdk::frame_support::sp_runtime::generic::Era;

    fn to_hex_string(xs: &[u8]) -> String {
        xs.iter().map(|b| format!("{b:02x}")).collect()
    }

    let era = Era::mortal(64, 0);
    let x = era.encode();
    assert_eq!(to_hex_string(&x), "0500");

    ...
  */
  // clang-format on

  // Test numeric limits for the block number, from 0 to u32::MAX.
  auto x = scale_encode_mortality(0, 64);
  EXPECT_EQ(base::HexEncodeLower(x), "0500");

  x = scale_encode_mortality(std::numeric_limits<uint32_t>::max(), 64);
  EXPECT_EQ(base::HexEncodeLower(x), "f503");

  // Test numeric limits for the period, which is defined to be 4 to 1 << 16.
  x = scale_encode_mortality(1234, 4);
  EXPECT_EQ(base::HexEncodeLower(x), "2100");

  x = scale_encode_mortality(1234, 3);
  EXPECT_EQ(base::HexEncodeLower(x), "2100");

  x = scale_encode_mortality(1234, 0);
  EXPECT_EQ(base::HexEncodeLower(x), "2100");

  x = scale_encode_mortality(1234, 1 << 16);
  EXPECT_EQ(base::HexEncodeLower(x), "df04");

  x = scale_encode_mortality(1234, (1 << 16) - 1);
  EXPECT_EQ(base::HexEncodeLower(x), "df04");

  x = scale_encode_mortality(1234, 1 << 17);
  EXPECT_EQ(base::HexEncodeLower(x), "df04");
}

TEST(PolkadotExtrinsics, SignaturePayload) {
  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  auto mainnet_metadata =
      PolkadotChainMetadata::FromChainName(kPolkadotChainType).value();

  uint32_t sender_nonce = 2;

  uint128_t send_amount = 1234;
  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> recipient = {};
  base::HexStringToSpan(kBob, recipient);

  uint32_t spec_version = 1020001;
  uint32_t transaction_version = 27;

  uint32_t block_number = 0x1b41217;

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(
      "0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e",
      genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(
      "0xbdcb3205ee391126e758556ffef5bb0d5a5fd1bbd996c671a079d5b02a671913",
      block_hash));

  auto encoded = generate_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, recipient,
      spec_version, transaction_version, block_number, genesis_hash,
      block_hash);

  constexpr const char kExpected[] =
      R"(0403008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913750108000061900f001b000000e143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423ebdcb3205ee391126e758556ffef5bb0d5a5fd1bbd996c671a079d5b02a67191300)";

  EXPECT_EQ(base::HexEncodeLower(encoded), kExpected);
}

TEST(PolkadotExtrinsics, SignedExtrinsic) {
  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  std::array<uint8_t, 32> recipient = {};
  base::HexStringToSpan(kBob, recipient);

  uint128_t send_amount = 1234;
  uint32_t spec_version = 1020001;
  uint32_t transaction_version = 27;

  uint32_t sender_nonce = 1;
  uint32_t block_number = 28794326;
  const char block_hash_encoded[] =
      R"(0x4b12cf2089483b06ea4fab577067bebe0e936dbb5317232d65617ab3af7fa425)";

  const char genesis_hash_encoded[] =
      R"(0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e)";

  PolkadotUnsignedTransfer transfer(recipient, send_amount);

  uint32_t account_index = 0;

  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  keypair = keypair.DeriveHard(base::byte_span_from_cstring("\x1cwestend"));
  keypair = keypair.DeriveHard(base::byte_span_from_ref(account_index));
  EXPECT_EQ(base::HexEncodeLower(keypair.GetPublicKey()),
            "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274");

  keypair.UseMockRngForTesting();

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash_encoded, genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(block_hash_encoded, block_hash));

  auto signature_payload = generate_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, recipient,
      spec_version, transaction_version, block_number, genesis_hash,
      block_hash);

  auto signature = keypair.SignMessage(signature_payload);

  EXPECT_TRUE(keypair.VerifyMessage(signature, signature_payload));

  const char expected_signatured[] =
      R"(2a1f28c7d292dd8ecbe9e461c318ab970582153bbb1c0a648d6d6961db064c1a284d223455cabaf9c5d0d8a77ed63178b6ccfba83cfb6dda61faa12062031486)";
  EXPECT_EQ(base::HexEncodeLower(signature), expected_signatured);

  auto signed_extrinsic = make_signed_extrinsic(
      *testnet_metadata, keypair.GetPublicKey(), recipient, send_amount_bytes,
      signature, block_number, sender_nonce);

  auto extrinsic = base::HexEncodeLower(signed_extrinsic);

  // clang-format off
  /*
    This extrinsic lives at the testnet here:
    https://westend.subscan.io/extrinsic/28794338-2

    This can be verified using:

      curl \
        -H "Content-Type: application/json" \
        -d "{\"id\":1, \"jsonrpc\":\"2.0\", \"method\": \"chain_getBlock\", \"params\": [\"0x2a569559aa25d5f70760d412af400fb6c05e9c7fed2b161a159b2873662f4f0a\"]}" \
      https://westend-rpc.polkadot.io

    The response should contain an array of committed transactions, including the one we have below:
    {
      "jsonrpc": "2.0",
      "id": 1,
      "result": {
        "block": {
          "header": {
            "parentHash": "0x8dcce8c540f1bf9a976f2aa060979156795b9d8c0b2db7c39060bc127c183685",
            "number": "0x1b75de2", // 28794338 in decimal.
            "stateRoot": "0x07c8cdac7003abc2487eca5c45b76f44a73a05d61ff4f6942ad004404178cfe6",
            "extrinsicsRoot": "0x192e5a4077f3a9c7c2bc8779188046fedea352e16622488488aa8cd015e94c56",
            "digest": {
              "logs": [
                "0x0642414245b5010313000000a18c8811000000000e42c1210f073986729415c04138fc8210574d116d45039f09a39755c6ac9f50b15416c75cb23c5cee9a9d85eca1c1e680693b26c53a29c2f4cf42c690c5b501fe302e8df120917052b1b1aa014441d2c02d94ec5c522afd1e5eb291dc8ad407",
                "0x04424545468403b9409da31bff3549cd873c7c9f283ba4a8ef2e87cfdaf0e8fc406c05bde8e583",
                "0x05424142450101c89d69dc34a07575db519d335e5f017ec5e770eee528a39c7c64b3dde8d5f618fb0808a143f082627d7176172c0f1c64b3ed122cab86f87215ce32402fc16380"
              ]
            }
          },
          "extrinsics": [
            "0x280502000b70fd5ff09a01",
            ...,
            "0x35028400d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b4927401441018831cb0c3977e5e15c1fe632cfb2eeb6147edef9c5d83005df0686fcb64358416735e42f72c0666f8b37fc53d55d4def2b321ef3e143480423ba70d938165010400000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913"
          ]
        },
        "justifications": null
      }
    }
  */
  // clang-format on

  std::string_view expected_extrinsic =
      "3502"  // SCALE-encoded length.
      "84"    // Sign bit set (0x80), extrinsic version (0x04).
      "00"    // MultiAddress type.
      "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274"
      "01"  // Signature type (sr25519).
      "2a1f28c7d292dd8ecbe9e461c318ab970582153bbb1c0a648d6d6961db064c1a"
      "284d223455cabaf9c5d0d8a77ed63178b6ccfba83cfb6dda61faa12062031486"
      "6501"  // MortalEra
      "04"    // SCALE-encoded nonce.
      "00"    // Tip.
      "00"    // Mode (disable metadata hash checking).
      "0403"  // Pallet index, call index.
      "00"    // MultiAddress type.
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48"
      "4913"  // SCALE-encoded send amount.
      ;

  EXPECT_EQ(extrinsic, expected_extrinsic);
}

TEST(PolkadotExtrinsics, UnsignedExtrinsicBase) {
  // Test that our UnsignedExtrinsic base class enables us to encode the
  // transfer extrinsic and that we can also decode it trivially.

  std::string_view testnet_extrinsic =
      R"(98040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  auto transfer_extrinsic =
      PolkadotUnsignedTransfer::Decode(testnet_metadata, testnet_extrinsic);

  EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
  EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()), kBob);

  PolkadotUnsignedExtrinsic& base = *transfer_extrinsic;

  EXPECT_EQ(base.Encode(testnet_metadata), testnet_extrinsic);

  transfer_extrinsic =
      PolkadotUnsignedExtrinsic::Decode<PolkadotUnsignedTransfer>(
          testnet_metadata, base.Encode(testnet_metadata));

  EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
  EXPECT_EQ(base::HexEncodeLower(transfer_extrinsic.value().recipient()), kBob);
}

TEST(PolkadotExtrinsics, MetadataSerde) {
  {
    // Test empty (de)serialization.

    PolkadotExtrinsicMetadata metadata;

    const char expected_json[] = R"(
    {
      "block_hash": "0000000000000000000000000000000000000000000000000000000000000000",
      "block_num": "00000000",
      "extrinsic": "",
      "mortality_period": "40000000"
    })";

    EXPECT_EQ(metadata.ToValue(), base::test::ParseJsonDict(expected_json));

    metadata = PolkadotExtrinsicMetadata::FromValue(
                   base::test::ParseJsonDict(expected_json))
                   .value();

    EXPECT_EQ(metadata.block_hash(),
              (std::array<uint8_t, kPolkadotBlockHashSize>{}));
    EXPECT_EQ(metadata.block_num(), 0u);
    EXPECT_EQ(metadata.extrinsic(), (std::vector<uint8_t>{}));
    EXPECT_EQ(metadata.mortality_period(), 64u);
  }

  {
    // Test non-empty (de)serialization.
    const char block_hash_hex[] =
        R"(c01286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a57e)";
    std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
    EXPECT_TRUE(base::HexStringToSpan(block_hash_hex, block_hash));

    const char extrinsic_hex[] =
        R"(4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c)";
    std::vector<uint8_t> extrinsic;
    EXPECT_TRUE(base::HexStringToBytes(extrinsic_hex, &extrinsic));

    PolkadotExtrinsicMetadata metadata;
    metadata.set_block_hash(block_hash);
    metadata.set_extrinsic(extrinsic);
    metadata.set_block_num(29959235);
    metadata.set_mortality_period(32);

    const char expected_json[] = R"(
    {
      "block_hash": "c01286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a57e",
      "block_num": "4324c901",
      "extrinsic": "4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c",
      "mortality_period": "20000000"
    })";

    EXPECT_EQ(metadata.ToValue(), base::test::ParseJsonDict(expected_json));

    metadata = PolkadotExtrinsicMetadata::FromValue(
                   base::test::ParseJsonDict(expected_json))
                   .value();

    EXPECT_EQ(metadata.block_hash(), block_hash);
    EXPECT_EQ(metadata.block_num(), 29959235u);
    EXPECT_EQ(metadata.extrinsic(), extrinsic);
    EXPECT_EQ(metadata.mortality_period(), 32u);
  }

  {
    // Block hash is non-hex.

    const char expected_json[] = R"(
    {
      "block_hash": "cat286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a57e",
      "block_num": "4324c901",
      "extrinsic": "4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c",
      "mortality_period": "20000000"
    })";

    auto metadata = PolkadotExtrinsicMetadata::FromValue(
        base::test::ParseJsonDict(expected_json));

    EXPECT_EQ(metadata, std::nullopt);
  }

  {
    // Block hash is too short.

    const char expected_json[] = R"(
    {
      "block_hash": "c01286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a5",
      "block_num": "4324c901",
      "extrinsic": "4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c",
      "mortality_period": "20000000"
    })";

    auto metadata = PolkadotExtrinsicMetadata::FromValue(
        base::test::ParseJsonDict(expected_json));

    EXPECT_EQ(metadata, std::nullopt);
  }

  {
    // Block hash is too long.

    const char expected_json[] = R"(
    {
      "block_hash": "c01286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a57eaa",
      "block_num": "4324c901",
      "extrinsic": "4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c",
      "mortality_period": "20000000"
    })";

    auto metadata = PolkadotExtrinsicMetadata::FromValue(
        base::test::ParseJsonDict(expected_json));

    EXPECT_EQ(metadata, std::nullopt);
  }

  {
    // Extrinsic is non-hex.

    const char expected_json[] = R"(
    {
      "block_hash": "c01286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a57e",
      "block_num": "4324c901",
      "extrinsic": "cat2840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c",
      "mortality_period": "20000000"
    })";

    auto metadata = PolkadotExtrinsicMetadata::FromValue(
        base::test::ParseJsonDict(expected_json));

    EXPECT_EQ(metadata, std::nullopt);
  }

  {
    // Block num exceeds numeric limits.

    const char expected_json[] = R"(
    {
      "block_hash": "c01286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a57e",
      "block_num": "4324c90101",
      "extrinsic": "4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c",
      "mortality_period": "20000000"
    })";

    auto metadata = PolkadotExtrinsicMetadata::FromValue(
        base::test::ParseJsonDict(expected_json));

    EXPECT_EQ(metadata, std::nullopt);
  }

  {
    // Morality period exceeds numeric limits.

    const char expected_json[] = R"(
    {
      "block_hash": "c01286d21f2b843c2fda8f7fd09a7f4eab1229d97041306536a7a9606961a57e",
      "block_num": "4324c901",
      "extrinsic": "4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c",
      "mortality_period": "2000000001"
    })";

    auto metadata = PolkadotExtrinsicMetadata::FromValue(
        base::test::ParseJsonDict(expected_json));

    EXPECT_EQ(metadata, std::nullopt);
  }
}

TEST(PolkadotExtrinsics, EventsParsing) {
  // This event comes from:
  // https://polkadot.subscan.io/extrinsic/30267458-2
  // Note that because the entire events blob for a block is ~12 kB, we choose
  // to only include a subset for this test.

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender = {};
  const char sender_hex[] =
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db";
  ASSERT_TRUE(base::HexStringToSpan(sender_hex, sender));

  auto chain_metadata = make_chain_metadata("Polkadot")->unwrap();

  const uint32_t extrinsic_idx = 2;

  const char events_hex[] =
      // balances(Withdraw)
      "0002000000"
      "0508"
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
      "5f139909000000000000000000000000"
      "00"
      // balances(Transfer)
      "0002000000"
      "0502"
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
      "5d70f7105a51be4a5afd2f10377d9bec9b8cdb971d6e8c436630f236a805926e"
      "a1d0724a020000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "6d6f646c70792f74727372790000000000000000000000000000000000000000"
      "18a9ad07000000000000000000000000"
      "00"
      // transactionpayment(TransactionFeePaid)
      "0002000000"
      "2000"
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
      "5f139909000000000000000000000000"
      "00000000000000000000000000000000"
      "00"
      // system(ExtrinsicSuccess)
      "0002000000"
      "0000"
      "a2e910976da80000"
      "00";

  std::vector<uint8_t> events;
  ASSERT_TRUE(base::HexStringToBytes(events_hex, &events));

  std::array<uint8_t, 16> actual_fee_bytes = {};

  EXPECT_TRUE(was_extrinsic_successful(rust::Slice<const uint8_t>(events),
                                       extrinsic_idx, sender, *chain_metadata,
                                       actual_fee_bytes));

  EXPECT_EQ(base::bit_cast<uint128_t>(actual_fee_bytes), uint128_t{161026911});
  EXPECT_EQ(base::HexEncodeLower(actual_fee_bytes),
            "5f139909000000000000000000000000");
}

TEST(PolkadotExtrinsics, EventsParsing_WithAccountCreation) {
  // This event comes from:
  // https://polkadot.subscan.io/extrinsic/30123219-2

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender = {};
  const char sender_hex[] =
      "2a27dd26f5f3fe4f48fc67cddb54a8cdb0f3c6e4b9c8cf751a59466771dc6144";
  ASSERT_TRUE(base::HexStringToSpan(sender_hex, sender));

  auto chain_metadata = make_chain_metadata("Polkadot")->unwrap();

  uint32_t extrinsic_idx = 2;

  const char events_hex[] =
      // balances(Withdraw)
      "0002000000"
      "0508"
      "2a27dd26f5f3fe4f48fc67cddb54a8cdb0f3c6e4b9c8cf751a59466771dc6144"
      "5f139909000000000000000000000000"
      "00"
      // system(NewAccount)
      "0002000000"
      "0003"
      "70617261550d0000000000000000000000000000000000000000000000000000"
      "00"
      // balances(Endowed)
      "0002000000"
      "0500"
      "70617261550d0000000000000000000000000000000000000000000000000000"
      "00d8bc7ced0000000000000000000000"
      "00"
      // balances(Transfer)
      "0002000000"
      "0502"
      "2a27dd26f5f3fe4f48fc67cddb54a8cdb0f3c6e4b9c8cf751a59466771dc6144"
      "70617261550d0000000000000000000000000000000000000000000000000000"
      "00d8bc7ced0000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "6d6f646c70792f74727372790000000000000000000000000000000000000000"
      "18a9ad07000000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "d8837440d77698a5ac63985587594e45d0029538f9b413495621913a68f64941"
      "476aeb01000000000000000000000000"
      "00"
      // transactionpayment(TransactionFeePaid)
      "0002000000"
      "2000"
      "2a27dd26f5f3fe4f48fc67cddb54a8cdb0f3c6e4b9c8cf751a59466771dc6144"
      "5f139909000000000000000000000000"
      "00000000000000000000000000000000"
      "00"
      // system(ExtrinsicSuccess)
      "0002000000"
      "0000"
      "a2e910976da80000"
      "00";

  std::vector<uint8_t> events;
  ASSERT_TRUE(base::HexStringToBytes(events_hex, &events));

  std::array<uint8_t, 16> actual_fee_bytes = {};

  EXPECT_TRUE(was_extrinsic_successful(rust::Slice<const uint8_t>(events),
                                       extrinsic_idx, sender, *chain_metadata,
                                       actual_fee_bytes));

  EXPECT_EQ(base::bit_cast<uint128_t>(actual_fee_bytes), uint128_t{161026911});
  EXPECT_EQ(base::HexEncodeLower(actual_fee_bytes),
            "5f139909000000000000000000000000");
}

TEST(PolkadotExtrinsics, EventsParsing_FailedExtrinsic_ArithmeticUnderflow) {
  // This event comes from:
  // https://polkadot.subscan.io/extrinsic/29943577-2

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender = {};
  const char sender_hex[] =
      "d44c4639d57190aed08f053cac6db1c85221253e7353d484dba9caa663d86a5f";
  ASSERT_TRUE(base::HexStringToSpan(sender_hex, sender));

  auto chain_metadata = make_chain_metadata("Polkadot")->unwrap();

  uint32_t extrinsic_idx = 2;

  const char events_hex[] =
      // balances(Withdraw)
      "0002000000"
      "0508"
      "d44c4639d57190aed08f053cac6db1c85221253e7353d484dba9caa663d86a5f"
      "9f5ee509000000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "6d6f646c70792f74727372790000000000000000000000000000000000000000"
      "18b2ea07000000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "e08785d4123f656862a5fd4b2286ae30ab1ebf17f60538961d3f91f02c73ee91"
      "87acfa01000000000000000000000000"
      "00"
      // transactionpayment(TransactionFeePaid)
      "0002000000"
      "2000"
      "d44c4639d57190aed08f053cac6db1c85221253e7353d484dba9caa663d86a5f"
      "9f5ee509000000000000000000000000"
      "00000000000000000000000000000000"
      "00"
      // system(ExtrinsicFailed)
      "0002000000"
      "0001"
      "0800a2e910976da80000"
      "00";

  std::vector<uint8_t> events;
  ASSERT_TRUE(base::HexStringToBytes(events_hex, &events));

  std::array<uint8_t, 16> actual_fee_bytes = {};

  EXPECT_FALSE(was_extrinsic_successful(rust::Slice<const uint8_t>(events),
                                        extrinsic_idx, sender, *chain_metadata,
                                        actual_fee_bytes));

  EXPECT_EQ(base::bit_cast<uint128_t>(actual_fee_bytes), uint128_t{166026911});
  EXPECT_EQ(base::HexEncodeLower(actual_fee_bytes),
            "9f5ee509000000000000000000000000");
}

TEST(PolkadotExtrinsics, EventsParsing_FailedExtrinsic_BelowMinimum) {
  // This event comes from:
  // https://polkadot.subscan.io/extrinsic/29509101-2

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender = {};
  const char sender_hex[] =
      "3c67dd0ea1126b09609ac341b4417251457f0fad467b8e1d3004209d4756ea2e";
  ASSERT_TRUE(base::HexStringToSpan(sender_hex, sender));

  auto chain_metadata = make_chain_metadata("Polkadot")->unwrap();

  uint32_t extrinsic_idx = 2;

  const char events_hex[] =
      // balances(Withdraw)
      "0002000000"
      "0508"
      "3c67dd0ea1126b09609ac341b4417251457f0fad467b8e1d3004209d4756ea2e"
      "5f139909000000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "6d6f646c70792f74727372790000000000000000000000000000000000000000"
      "18a9ad07000000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "c5e80accf4092ea6f8ed087544576dddcfdd51366b492868f73b0c9ca19c5f31"
      "476aeb01000000000000000000000000"
      "00"
      // transactionpayment(TransactionFeePaid)
      "0002000000"
      "2000"
      "3c67dd0ea1126b09609ac341b4417251457f0fad467b8e1d3004209d4756ea2e"
      "5f139909000000000000000000000000"
      "00000000000000000000000000000000"
      "00"
      // system(ExtrinsicFailed)
      "0002000000"
      "0001"
      "0702a2e910976da80000"
      "00";

  std::vector<uint8_t> events;
  ASSERT_TRUE(base::HexStringToBytes(events_hex, &events));

  std::array<uint8_t, 16> actual_fee_bytes = {};

  EXPECT_FALSE(was_extrinsic_successful(rust::Slice<const uint8_t>(events),
                                        extrinsic_idx, sender, *chain_metadata,
                                        actual_fee_bytes));

  EXPECT_EQ(base::bit_cast<uint128_t>(actual_fee_bytes), uint128_t{161026911});
  EXPECT_EQ(base::HexEncodeLower(actual_fee_bytes),
            "5f139909000000000000000000000000");
}

TEST(PolkadotExtrinsics, EventsParsing_Error) {
  const char sender_hex[] =
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db";

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender = {};

  ASSERT_TRUE(base::HexStringToSpan(sender_hex, sender));

  auto chain_metadata = make_chain_metadata("Polkadot")->unwrap();

  uint32_t extrinsic_idx = 2;

  const std::string valid_events =
      // balances(Withdraw)
      "0002000000"
      "0508"
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
      "5f139909000000000000000000000000"
      "00"
      // balances(Transfer)
      "0002000000"
      "0502"
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
      "5d70f7105a51be4a5afd2f10377d9bec9b8cdb971d6e8c436630f236a805926e"
      "a1d0724a020000000000000000000000"
      "00"
      // balances(Deposit)
      "0002000000"
      "0507"
      "6d6f646c70792f74727372790000000000000000000000000000000000000000"
      "18a9ad07000000000000000000000000"
      "00"
      // transactionpayment(TransactionFeePaid)
      "0002000000"
      "2000"
      "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
      "5f139909000000000000000000000000"
      "00000000000000000000000000000000"
      "00"
      // system(ExtrinsicSuccess)
      "0002000000"
      "0000"
      "a2e910976da80000"
      "00";

  std::vector<std::string> inputs;

  // Incorrect transactionpayment(TransactionFeePaid).
  {
    std::string bad_fee_paid_event_prefix = valid_events;
    auto n = bad_fee_paid_event_prefix.find("00020000002000");
    bad_fee_paid_event_prefix[n] = '1';
    inputs.push_back(std::move(bad_fee_paid_event_prefix));
  }
  {
    std::string bad_fee_paid_event_prefix = valid_events;
    auto n = bad_fee_paid_event_prefix.find("00020000002000");
    bad_fee_paid_event_prefix.erase(n + 5, 2);
    inputs.push_back(std::move(bad_fee_paid_event_prefix));
  }

  // Incorrect system(ExtrinsicSuccess).
  {
    std::string bad_extrinsic_success_event_prefix = valid_events;
    auto n = bad_extrinsic_success_event_prefix.find("00020000000000");
    bad_extrinsic_success_event_prefix[n] = '1';
    inputs.push_back(std::move(bad_extrinsic_success_event_prefix));
  }
  {
    std::string bad_extrinsic_success_event_prefix = valid_events;
    auto n = bad_extrinsic_success_event_prefix.find("00020000000000");
    bad_extrinsic_success_event_prefix.erase(n + 5, 2);
    inputs.push_back(std::move(bad_extrinsic_success_event_prefix));
  }

  // Incorrect sender in transaction fee paid.
  {
    std::string bad_sender_transfer = valid_events;
    auto n = bad_sender_transfer.find(sender_hex);
    n = bad_sender_transfer.find(sender_hex, n + 64);
    n = bad_sender_transfer.find(sender_hex, n + 64);
    bad_sender_transfer[n] = '0';
    inputs.push_back(std::move(bad_sender_transfer));
  }

  // Incorrect topics for fee paid.
  {
    std::string needle =
        "5f139909000000000000000000000000"
        "00000000000000000000000000000000"
        "00";

    std::string bad_fee_paid_topics = valid_events;
    auto n = bad_fee_paid_topics.find(needle);
    bad_fee_paid_topics[n + needle.size() - 2] = '1';

    inputs.push_back(std::move(bad_fee_paid_topics));
  }

  // Extrinsic indexes don't match.
  {
    std::string invalid_event =
        // balances(Withdraw)
        "0003000000"
        "0508"
        "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
        "5f139909000000000000000000000000"
        "00"
        // balances(Transfer)
        "0003000000"
        "0502"
        "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
        "5d70f7105a51be4a5afd2f10377d9bec9b8cdb971d6e8c436630f236a805926e"
        "a1d0724a020000000000000000000000"
        "00"
        // balances(Deposit)
        "0003000000"
        "0507"
        "6d6f646c70792f74727372790000000000000000000000000000000000000000"
        "18a9ad07000000000000000000000000"
        "00"
        // transactionpayment(TransactionFeePaid)
        "0003000000"
        "2000"
        "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
        "5f139909000000000000000000000000"
        "00000000000000000000000000000000"
        "00"
        // system(ExtrinsicSuccess)
        "0003000000"
        "0000"
        "a2e910976da80000"
        "00";

    inputs.push_back(std::move(invalid_event));
  }

  // Empty string.
  {
    inputs.push_back("");
  }

  // Truncated TransactionFeePaid.
  {
    std::string needle =
        "5f139909000000000000000000000000"
        "00000000000000000000000000000000"
        "00";

    std::string truncated = valid_events;
    auto n = truncated.find(needle);
    truncated.erase(n);

    inputs.push_back(std::move(truncated));
  }

  ASSERT_FALSE(inputs.empty());
  for (const auto& input : inputs) {
    std::vector<uint8_t> events;
    if (!input.empty()) {
      ASSERT_TRUE(base::HexStringToBytes(input, &events));
    }

    std::array<uint8_t, 16> actual_fee_bytes = {};

    EXPECT_FALSE(was_extrinsic_successful(rust::Slice<const uint8_t>(events),
                                          extrinsic_idx, sender,
                                          *chain_metadata, actual_fee_bytes))
        << input;

    EXPECT_EQ(base::bit_cast<uint128_t>(actual_fee_bytes), uint128_t{0});
  }
}

}  // namespace brave_wallet
