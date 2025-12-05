/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"

#include "base/strings/string_number_conversions.h"
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
      R"(0400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913750108000061900f001b000000e143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423ebdcb3205ee391126e758556ffef5bb0d5a5fd1bbd996c671a079d5b02a67191300)";

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
      "441018831cb0c3977e5e15c1fe632cfb2eeb6147edef9c5d83005df0686fcb64"
      "358416735e42f72c0666f8b37fc53d55d4def2b321ef3e143480423ba70d9381"
      "6501"  // MortalEra
      "04"    // SCALE-encoded nonce.
      "00"    // Tip.
      "00"    // Mode (disable metadata hash checking).
      "0400"  // Pallet index, call index.
      "00"    // MultiAddress type.
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48"
      "4913"  // SCALE-encoded send amount.
      ;

  EXPECT_EQ(extrinsic, expected_extrinsic);
}

}  // namespace brave_wallet
