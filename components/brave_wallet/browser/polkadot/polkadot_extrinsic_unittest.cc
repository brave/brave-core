/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "8EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A48";

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

}  // namespace

TEST(PolkadotExtrinsics, UnsignedTransfer) {
  // Test we can construct an unsigned extrinsic representing a
  // transfer_allow_death call and then serialize it appropriately to a hex
  // string.

  PolkadotChainMetadata metadata;
  metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendChainType);
  metadata.AddChainMetadata(mojom::kPolkadotMainnet, kPolkadotChainType);

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
      R"(98040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

  EXPECT_EQ(transfer_extrinsic.send_amount(), 1234u);
  EXPECT_EQ(base::HexEncode(transfer_extrinsic.recipient()), kBob);
  EXPECT_EQ(transfer_extrinsic.Encode(mojom::kPolkadotTestnet, metadata),
            testnet_extrinsic);

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
      R"(98040500008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

  EXPECT_EQ(transfer_extrinsic.Encode(mojom::kPolkadotMainnet, metadata),
            mainnet_extrinsic);
}

TEST(PolkadotExtrinsics, UnsignedTransferAssetHub) {
  // Test that when our code is setup to point to a different parachain, such as
  // AssetHub, that we can still generate an extrinsic with the correct pallet
  // and call indices.

  PolkadotChainMetadata metadata;
  metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendAssetHubChainType);
  metadata.AddChainMetadata(mojom::kPolkadotMainnet,
                            kPolkadotAssetHubChainType);

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
      R"(98040A00008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

  EXPECT_EQ(transfer_extrinsic.send_amount(), 1234u);
  EXPECT_EQ(base::HexEncode(transfer_extrinsic.recipient()), kBob);
  EXPECT_EQ(transfer_extrinsic.Encode(mojom::kPolkadotTestnet, metadata),
            testnet_extrinsic);

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
      R"(98040A00008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

  EXPECT_EQ(transfer_extrinsic.Encode(mojom::kPolkadotMainnet, metadata),
            mainnet_extrinsic);
}

TEST(PolkadotExtrinsics, UnsignedTransferNumericLimits) {
  // Test our extrinsic creation and serialization using numeric limits for a
  // uint128_t.

  PolkadotChainMetadata metadata;
  metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendChainType);
  metadata.AddChainMetadata(mojom::kPolkadotMainnet, kPolkadotChainType);

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
        R"(D4040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A4833FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)";
    EXPECT_EQ(transfer_extrinsic.Encode(mojom::kPolkadotTestnet, metadata),
              testnet_extrinsic);
  }

  {
    uint128_t send_amount = 0;
    PolkadotUnsignedTransfer transfer_extrinsic(pubkey, send_amount);

    const char* testnet_extrinsic =
        R"(94040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A4800)";
    EXPECT_EQ(transfer_extrinsic.Encode(mojom::kPolkadotTestnet, metadata),
              testnet_extrinsic);
  }
}

TEST(PolkadotExtrinsics, DecodedUnsignedTransfer) {
  // Test that we can appropriately decode the hex representation of an
  // extrinsic for a given relay chain.

  PolkadotChainMetadata metadata;
  metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendChainType);
  metadata.AddChainMetadata(mojom::kPolkadotMainnet, kPolkadotChainType);

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
        R"(98040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

    auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
        mojom::kPolkadotTestnet, metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncode(transfer_extrinsic.value().recipient()), kBob);
  }

  {
    const char* mainnet_extrinsic =
        R"(98040500008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

    auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
        mojom::kPolkadotMainnet, metadata, mainnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncode(transfer_extrinsic.value().recipient()), kBob);
  }
}

TEST(PolkadotExtrinsics, DecodedUnsignedTransferAssetHub) {
  // Test that we can decode extrinsics for a specific parachain, like AssetHub.

  PolkadotChainMetadata metadata;
  metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendAssetHubChainType);
  metadata.AddChainMetadata(mojom::kPolkadotMainnet,
                            kPolkadotAssetHubChainType);

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
        R"(98040A00008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

    auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
        mojom::kPolkadotTestnet, metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncode(transfer_extrinsic.value().recipient()), kBob);
  }

  {
    const char* mainnet_extrinsic =
        R"(98040A00008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

    auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
        mojom::kPolkadotMainnet, metadata, mainnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncode(transfer_extrinsic.value().recipient()), kBob);
  }
}

TEST(PolkadotExtrinsics, DecodeNumericLimits) {
  // Test extrinsic decoding for numeric limits.

  PolkadotChainMetadata metadata;
  metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendChainType);
  metadata.AddChainMetadata(mojom::kPolkadotMainnet, kPolkadotChainType);

  {
    const char* testnet_extrinsic =
        R"(D4040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A4833FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)";

    auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
        mojom::kPolkadotTestnet, metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(),
              std::numeric_limits<uint128_t>::max());
    EXPECT_EQ(base::HexEncode(transfer_extrinsic.value().recipient()), kBob);
  }

  {
    const char testnet_extrinsic[] =
        R"(94040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A4800)";

    auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
        mojom::kPolkadotTestnet, metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 0u);
    EXPECT_EQ(base::HexEncode(transfer_extrinsic.value().recipient()), kBob);
  }
}

TEST(PolkadotExtrinsics, InvalidDecode) {
  // Test that subtle differences in the hex-encoded extrinsics will cause our
  // code to fail to parse.

  PolkadotChainMetadata metadata;
  metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendChainType);
  metadata.AddChainMetadata(mojom::kPolkadotMainnet, kPolkadotChainType);

  {
    // Valid data, but not enough.

    std::string_view mainnet_extrinsic =
        R"(98040500008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

    for (size_t i = 0; i < mainnet_extrinsic.size() - 1; ++i) {
      auto input = mainnet_extrinsic.substr(0, i);
      auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
          mojom::kPolkadotMainnet, metadata, input);
      EXPECT_FALSE(transfer_extrinsic) << input;
    }
  }

  {
    std::string_view inputs[] = {
        R"(55040500008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)",  // Invalid leading length.
        R"(98110500008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)",  // Invalid extrinsic version.
        R"(98041200008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)",  // Invalid pallet index.
        R"(98040534008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)",  // Invalid call index.
        R"(98040500018EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)",  // Invalid MultiAddress type.
        R"(98040500008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A481234)",  // Invalid send amount.
        R"(98040500008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A4833FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)",  // Send amount would exceed numeric limits.
    };

    for (auto input : inputs) {
      auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
          mojom::kPolkadotMainnet, metadata, input);
      EXPECT_FALSE(transfer_extrinsic) << input;
    }
  }
}

TEST(PolkadotExtrinsics, InvalidDecodeFromIncompatibleParachain) {
  // Test the case where we have a valid extrinsics for a specific parachain
  // that's not compatible with the current one we're supplying.

  const char* testnet_extrinsic =
      R"(98040400008EAF04151687736326C9FEA17E25FC5287613693C912909CB226AA4794F26A484913)";

  {
    PolkadotChainMetadata metadata;
    metadata.AddChainMetadata(mojom::kPolkadotTestnet, kWestendChainType);
    metadata.AddChainMetadata(mojom::kPolkadotMainnet, kPolkadotChainType);

    auto transfer_extrinsic = PolkadotUnsignedTransfer::Decode(
        mojom::kPolkadotTestnet, metadata, testnet_extrinsic);

    EXPECT_EQ(transfer_extrinsic.value().send_amount(), 1234u);
    EXPECT_EQ(base::HexEncode(transfer_extrinsic.value().recipient()), kBob);

    EXPECT_FALSE(PolkadotUnsignedTransfer::Decode(mojom::kPolkadotMainnet,
                                                  metadata, testnet_extrinsic));
  }

  {
    PolkadotChainMetadata metadata;
    metadata.AddChainMetadata(mojom::kPolkadotTestnet,
                              kWestendAssetHubChainType);
    metadata.AddChainMetadata(mojom::kPolkadotMainnet,
                              kPolkadotAssetHubChainType);

    EXPECT_FALSE(PolkadotUnsignedTransfer::Decode(mojom::kPolkadotTestnet,
                                                  metadata, testnet_extrinsic));
    EXPECT_FALSE(PolkadotUnsignedTransfer::Decode(mojom::kPolkadotMainnet,
                                                  metadata, testnet_extrinsic));
  }
}

}  // namespace brave_wallet
