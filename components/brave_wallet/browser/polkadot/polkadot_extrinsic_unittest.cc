/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48";

// Taken from:
// https://docs.rs/schnorrkel/0.11.4/schnorrkel/keys/struct.MiniSecretKey.html#method.from_bytes
constexpr uint8_t kSchnorrkelSeed[] = {
    157, 97,  177, 157, 239, 253, 90, 96,  186, 132, 74,
    244, 146, 236, 44,  196, 68,  73, 197, 105, 123, 50,
    105, 25,  112, 59,  172, 3,   28, 174, 127, 96,
};

inline constexpr char kAssetHubMnemonic[] =
    "lazy february across turn unique syrup gasp pass pelican achieve cable "
    "canal";

}  // namespace

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
  auto testnet_metadata = MakeWestendMetadata();

  auto mainnet_metadata = MakePolkadotMetadata();

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
      *testnet_metadata, sender_nonce, send_amount_bytes,
      /*transfer_all=*/false, recipient, spec_version, transaction_version,
      block_number, genesis_hash, block_hash);

  constexpr const char kExpected[] =
      R"(0403008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913750108000061900f001b000000e143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423ebdcb3205ee391126e758556ffef5bb0d5a5fd1bbd996c671a079d5b02a67191300)";

  EXPECT_EQ(base::HexEncodeLower(encoded), kExpected);
}

TEST(PolkadotExtrinsics, SignedExtrinsic_TransferKeepAlive) {
  auto testnet_metadata = MakeWestendMetadata();

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

  uint32_t account_index = 0;

  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  keypair = keypair.DeriveHard(base::byte_span_from_cstring("westend"));
  keypair = keypair.DeriveHard(account_index);
  EXPECT_EQ(base::HexEncodeLower(keypair.GetPublicKey()),
            "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274");

  keypair.SetMockRndSeedForTesting();

  const bool transfer_all = false;

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash_encoded, genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(block_hash_encoded, block_hash));

  auto signature_payload = generate_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, transfer_all,
      recipient, spec_version, transaction_version, block_number, genesis_hash,
      block_hash);

  auto signature = keypair.SignMessage(signature_payload);

  EXPECT_TRUE(keypair.VerifyMessage(signature, signature_payload));

  const char expected_signatured[] =
      R"(2a1f28c7d292dd8ecbe9e461c318ab970582153bbb1c0a648d6d6961db064c1a284d223455cabaf9c5d0d8a77ed63178b6ccfba83cfb6dda61faa12062031486)";
  EXPECT_EQ(base::HexEncodeLower(signature), expected_signatured);

  auto signed_extrinsic = make_signed_extrinsic(
      *testnet_metadata, keypair.GetPublicKey(), recipient, send_amount_bytes,
      transfer_all, signature, block_number, sender_nonce);

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

TEST(PolkadotExtrinsics, SignedExtrinsic_TransferAll) {
  auto testnet_metadata = MakeWestendMetadata();

  std::array<uint8_t, 32> recipient = {};
  base::HexStringToSpan(kBob, recipient);

  uint128_t send_amount = 1234;
  uint32_t spec_version = 1020001;
  uint32_t transaction_version = 27;

  uint32_t sender_nonce = 45;
  uint32_t block_number = 30508078;
  const char block_hash_encoded[] =
      R"(0x077a7467ddf9f37d0ebda40d830efcf4e895a599cc8cadfcd3e73588c5e70f82)";

  const char genesis_hash_encoded[] =
      R"(0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e)";

  uint32_t account_index = 0;

  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  keypair = keypair.DeriveHard(base::byte_span_from_cstring("westend"));
  keypair = keypair.DeriveHard(account_index);
  EXPECT_EQ(base::HexEncodeLower(keypair.GetPublicKey()),
            "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274");

  keypair.SetMockRndSeedForTesting();

  const bool transfer_all = true;

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash_encoded, genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(block_hash_encoded, block_hash));

  auto signature_payload = generate_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, transfer_all,
      recipient, spec_version, transaction_version, block_number, genesis_hash,
      block_hash);

  auto signature = keypair.SignMessage(signature_payload);

  EXPECT_TRUE(keypair.VerifyMessage(signature, signature_payload));

  const char expected_signatured[] =
      R"(0442eb6ee79e19a958e614a854a496303200c95a8420b378b1a0b1f0ae1949335b3e5e675ffd1d905ffd91f7b9ea5e9f9f92faba18607c79d72d9a428e5e8383)";
  EXPECT_EQ(base::HexEncodeLower(signature), expected_signatured);

  auto signed_extrinsic = make_signed_extrinsic(
      *testnet_metadata, keypair.GetPublicKey(), recipient, send_amount_bytes,
      transfer_all, signature, block_number, sender_nonce);

  auto extrinsic = base::HexEncodeLower(signed_extrinsic);

  std::string_view expected_extrinsic =
      "3102"  // SCALE-encoded length.
      "84"    // Sign bit set (0x80), extrinsic version (0x04).
      "00"    // MultiAddress type.
      "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274"
      "01"  // Signature type (sr25519).
      "0442eb6ee79e19a958e614a854a496303200c95a8420b378b1a0b1f0ae194933"
      "5b3e5e675ffd1d905ffd91f7b9ea5e9f9f92faba18607c79d72d9a428e5e8383"
      "e502"  // MortalEra
      "b4"    // SCALE-encoded nonce.
      "00"    // Tip.
      "00"    // Mode (disable metadata hash checking).
      "0404"  // Pallet index, call index.
      "00"    // MultiAddress type.
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48"
      "00"  // Keep-alive.
      ;

  EXPECT_EQ(extrinsic, expected_extrinsic);
}

TEST(PolkadotExtrinsics, SignedExtrinsic_TransferKeepAlive_AssetId) {
  auto testnet_metadata = MakeWestendAssetHubMetadata();

  const char recipient_hex[] =
      R"(2a2e19ed46a3875095b184d1d853085f732c1a56326fc25ebf7fc524904da05b)";

  std::array<uint8_t, 32> recipient = {};
  ASSERT_TRUE(base::HexStringToSpan(recipient_hex, recipient));

  uint128_t send_amount = 500000000000;
  uint32_t spec_version = 1022005;
  uint32_t transaction_version = 16;

  uint32_t sender_nonce = 23;
  uint32_t block_number = 14843538;
  const char block_hash_encoded[] =
      R"(0xebb9782a456b98d387233e8273a9e9d56efd719c6aad65c650dc4fc90a0ff099)";

  const char genesis_hash_encoded[] =
      R"(0x67f9723393ef76214df0118c34bbbd3dbebc8ed46a10973a8c969d48fe7598c9)";

  uint32_t account_index = 0;

  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  keypair = keypair.DeriveHard(base::byte_span_from_cstring("westend"));
  keypair = keypair.DeriveHard(account_index);
  EXPECT_EQ(base::HexEncodeLower(keypair.GetPublicKey()),
            "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274");

  keypair.SetMockRndSeedForTesting();

  const bool transfer_all = false;

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash_encoded, genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(block_hash_encoded, block_hash));

  auto signature_payload = generate_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, transfer_all,
      recipient, spec_version, transaction_version, block_number, genesis_hash,
      block_hash);

  auto signature = keypair.SignMessage(signature_payload);

  EXPECT_TRUE(keypair.VerifyMessage(signature, signature_payload));

  const char expected_signatured[] =
      R"(de8379bdff3b46793aafb9ef811a21e3014b9417cc288a974369533ad864d32d30e52d839c8bfae650f0e1940de1c354e22293febc63605a73dd01d8e59ec485)";
  EXPECT_EQ(base::HexEncodeLower(signature), expected_signatured);

  auto signed_extrinsic = make_signed_extrinsic(
      *testnet_metadata, keypair.GetPublicKey(), recipient, send_amount_bytes,
      transfer_all, signature, block_number, sender_nonce);

  auto extrinsic = base::HexEncodeLower(signed_extrinsic);

  /*
    Extrinsic lives here:
    https://assethub-westend.subscan.io/tx/0xa08ca7b57a6115160a365d62d8bc48d25a1e1fc687e2514888edaeed0d4b2aed

    Real extrinsic pulled from block using:

    curl.exe
      -H "Content-Type: application/json"
      -d
      "{\"id\":10,\"jsonrpc\":\"2.0\",\"method\":\"chain_getBlock\",\"params\":[\"0xb13147801054a1087a9b692b324d23c6b464f777387feb6c35a02d2c5eb4d36a\"]}"
      https://westend-asset-hub-rpc.polkadot.io

    4902
    84
    00
    52707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c
    01
    5a874d8382a24bfe79ff713903c7ee0fd80165f15a39e4f506bf025141826a64
    1eeaa0ba4bf268997592f7ddbe03f8f6e1c81d91bba03700cfa24dbc98dea486
    2501
    5c
    00
    00
    00
    0a03
    00
    2a2e19ed46a3875095b184d1d853085f732c1a56326fc25ebf7fc524904da05b
    070088526a74
  */

  std::string_view expected_extrinsic =
      "4902"  // SCALE-encoded length.
      "84"    // Signed, extrinsic version v4.
      "00"    // Multi-address type.
      "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274"
      "01"  // Signature type (sr25519).
      "de8379bdff3b46793aafb9ef811a21e3014b9417cc288a974369533ad864d32d"
      "30e52d839c8bfae650f0e1940de1c354e22293febc63605a73dd01d8e59ec485"
      "2501"  // Mortal era.
      "5c"    // SCALE-encoded nonce.
      "00"    // Tip.
      "00"    // Asset ID.
      "00"    // Mode (disable metadata hash checking).
      "0a03"  // Pallet index, call index.
      "00"    // Address type.
      "2a2e19ed46a3875095b184d1d853085f732c1a56326fc25ebf7fc524904da05b"
      "070088526a74"  // SCALE-encoded send amount
      ;

  EXPECT_EQ(extrinsic, expected_extrinsic);
}

TEST(PolkadotExtrinsics, SignedExtrinsic_TransferAll_AssetId) {
  auto testnet_metadata = MakeWestendAssetHubMetadata();

  const char recipient_hex[] =
      R"(52707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c)";

  std::array<uint8_t, 32> recipient = {};
  ASSERT_TRUE(base::HexStringToSpan(recipient_hex, recipient));

  uint128_t send_amount = 500000000000;
  uint32_t spec_version = 1022005;
  uint32_t transaction_version = 16;

  uint32_t sender_nonce = 0;
  uint32_t block_number = 14845178;
  const char block_hash_encoded[] =
      R"(0x361e5966fcc99daf038aa155971c167545e09ab7fa170b847aa9f868f967c065)";

  const char genesis_hash_encoded[] =
      R"(0x67f9723393ef76214df0118c34bbbd3dbebc8ed46a10973a8c969d48fe7598c9)";

  uint32_t account_index = 0;

  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  keypair = keypair.DeriveHard(base::byte_span_from_cstring("westend"));
  keypair = keypair.DeriveHard(account_index);
  EXPECT_EQ(base::HexEncodeLower(keypair.GetPublicKey()),
            "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274");

  keypair.SetMockRndSeedForTesting();

  const bool transfer_all = true;

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash_encoded, genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(block_hash_encoded, block_hash));

  auto signature_payload = generate_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, transfer_all,
      recipient, spec_version, transaction_version, block_number, genesis_hash,
      block_hash);

  auto signature = keypair.SignMessage(signature_payload);

  EXPECT_TRUE(keypair.VerifyMessage(signature, signature_payload));

  const char expected_signatured[] =
      R"(bac21dbe3fc9544b429378d054632bff8701e6a2385b617b2c86c8534ab98459d2fd7a98b8da4180694e8032f65a3f574bdb41e8566e70e97677c09fb741328e)";
  EXPECT_EQ(base::HexEncodeLower(signature), expected_signatured);

  auto signed_extrinsic = make_signed_extrinsic(
      *testnet_metadata, keypair.GetPublicKey(), recipient, send_amount_bytes,
      transfer_all, signature, block_number, sender_nonce);

  auto extrinsic = base::HexEncodeLower(signed_extrinsic);

  /*
    Extrinsic lives here:
    https://assethub-westend.subscan.io/tx/0x290ef8f1c3689862427bb91653b9d398fe2001ed832c51d39402e64a0005244c

    Real extrinsic pulled from block using:

    curl.exe
      -H "Content-Type: application/json"
      -d
      "{\"id\":10,\"jsonrpc\":\"2.0\",\"method\":\"chain_getBlock\",\"params\":[\"0x49c035e2befaa5f7406f702698b460ebab30f9b1047b7fa6629fc884f086302a\"]}"
      https://westend-asset-hub-rpc.polkadot.io

    3502
    84
    00
    2a2e19ed46a3875095b184d1d853085f732c1a56326fc25ebf7fc524904da05b
    01
    901e7f007229fa35b5998839a77657097ec4c7bb813d5c99b45796d5f236d942
    575473fbb5bd0481ad2a964d28d859b418026e5572471c8c50cf193865b4a880
    a503
    00
    00
    00
    00
    0a04
    00
    52707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c
    00
  */

  std::string_view expected_extrinsic =
      "3502"  // SCALE-encoded length.
      "84"    // Signed, extrinsic version v4.
      "00"    // Multi-address type.
      "d4f9c4dfa3e6ff57b4e1fdea8699e57b0210cf04afe0281acba187d7d1b49274"
      "01"  // Signature type (sr25519).
      "bac21dbe3fc9544b429378d054632bff8701e6a2385b617b2c86c8534ab98459"
      "d2fd7a98b8da4180694e8032f65a3f574bdb41e8566e70e97677c09fb741328e"
      "a503"  // Mortal era.
      "00"    // SCALE-encoded nonce.
      "00"    // Tip.
      "00"    // Asset ID.
      "00"    // Mode (disable metadata hash checking).
      "0a04"  // Pallet index, call index.
      "00"    // Address type.
      "52707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c"
      "00"  // Keep-alive false.
      ;

  EXPECT_EQ(extrinsic, expected_extrinsic);
}

TEST(PolkadotExtrinsics, SignedExtrinsic_AssetsTransferKeepAlive) {
  auto testnet_metadata = MakePaseoAssetHubMetadata();

  // Our extrinsic lives here:
  // https://assethub-paseo.subscan.io/extrinsic/10464509-2

  // Confirm extrinsic presence with:
  // curl -H "Content-Type: application/json" -d
  // '{"id":1,"jsonrpc":"2.0","method":"chain_getBlock","params":["0x7eee4f792fd223d51591d6963b7e7098f18ac515b74ac3d49f6d3cf4557bf69e"]}'
  // https://asset-hub-paseo-rpc.n.dwellir.com/

  const char recipient_hex[] =
      R"(ae70948d0c015b6c2b1ac46b8931ad6301f2c648f3f0adf71d08a68fe745561e)";

  std::array<uint8_t, 32> recipient = {};
  ASSERT_TRUE(base::HexStringToSpan(recipient_hex, recipient));

  uint32_t asset_id = 50001010;
  uint128_t send_amount = 1000000;
  uint32_t spec_version = 2002002;
  uint32_t transaction_version = 15;

  uint32_t sender_nonce = 4;
  uint32_t block_number = 10464506;
  const char block_hash_encoded[] =
      R"(0x6998c30ffb64a94ed53b97f4a2a72e667a7c909571c1ff4879cf1acb59142817)";

  const char genesis_hash_encoded[] =
      R"(0xd6eec26135305a8ad257a20d003357284c8aa03d0bdb2b357ab0a22371e11ef2)";

  auto seed = bip39::MnemonicToEntropyToSeed(kAssetHubMnemonic);
  ASSERT_TRUE(seed.has_value());

  auto polkadot_seed = base::span(seed.value()).first<kPolkadotSeedSize>();

  auto keypair = HDKeySr25519::GenerateFromSeed(polkadot_seed);
  EXPECT_EQ(base::HexEncodeLower(keypair.GetPublicKey()),
            "0e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e");

  keypair.SetMockRndSeedForTesting();

  const bool transfer_all = false;

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash_encoded, genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(block_hash_encoded, block_hash));

  auto signature_payload = generate_assets_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, transfer_all,
      recipient, asset_id, spec_version, transaction_version, block_number,
      genesis_hash, block_hash);

  auto signature = keypair.SignMessage(signature_payload);

  EXPECT_TRUE(keypair.VerifyMessage(signature, signature_payload));

  const char expected_signatured[] =
      R"(908e912c55c81dbbc69e05ae8f22793c2e32042601c6defe6a1e6cdd9cc14d554e579945a51524303d8a9ea95b9194085742aa7a4dd548e7718541fa8e5dba82)";
  EXPECT_EQ(base::HexEncodeLower(signature), expected_signatured);

  auto signed_extrinsic = make_signed_asset_transfer_extrinsic(
      *testnet_metadata, keypair.GetPublicKey(), recipient, send_amount_bytes,
      transfer_all, signature, block_number, sender_nonce, asset_id);

  auto extrinsic = base::HexEncodeLower(signed_extrinsic);

  std::string_view expected_extrinsic =
      "5102"  // SCALE-encoded length.
      "84"    // Signed, extrinsic v4.
      "00"    // Multi-address type.
      // Sender.
      "0e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e"
      "01"  // Signature type (sr25519).
      // Signature.
      "908e912c55c81dbbc69e05ae8f22793c2e32042601c6defe6a1e6cdd9cc14d55"
      "4e579945a51524303d8a9ea95b9194085742aa7a4dd548e7718541fa8e5dba82"
      "a503"      // Mortal era.
      "10"        // SCALE-encoded nonce.
      "00"        // Tip.
      "00"        // Asset ID for fee payment.
      "00"        // Mode.
      "3209"      // Pallet index, call index.
      "cad1eb0b"  // Scale-encoded asset id.
      "00"        // Address type
      // Recipient
      "ae70948d0c015b6c2b1ac46b8931ad6301f2c648f3f0adf71d08a68fe745561e"
      "02093d00"  // SCALE-encoded send amount.
      ;

  EXPECT_EQ(extrinsic, expected_extrinsic);
}

TEST(PolkadotExtrinsics, SignedExtrinsic_AssetsTransferAll) {
  auto testnet_metadata = MakePaseoAssetHubMetadata();

  // Our extrinsic lives here:
  // https://assethub-paseo.subscan.io/extrinsic/10545385-2

  // Confirm extrinsic presence with:
  // curl -H "Content-Type: application/json" -d
  // '{"id":1,"jsonrpc":"2.0","method":"chain_getBlock","params":["0x1742561de288726ae2bc266cadd80bb93e11f7fbf93a7cd88fe2da5f1489e621"]}'
  // https://asset-hub-paseo-rpc.n.dwellir.com/

  const char recipient_hex[] =
      R"(0e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e)";

  std::array<uint8_t, 32> recipient = {};
  ASSERT_TRUE(base::HexStringToSpan(recipient_hex, recipient));

  uint32_t asset_id = 50001010;
  uint128_t send_amount = 0;
  uint32_t spec_version = 2003001;
  uint32_t transaction_version = 15;

  uint32_t sender_nonce = 0;
  uint32_t block_number = 10545382;
  const char block_hash_encoded[] =
      R"(0xc67460694f5a469493914f72eadf4b688e4c842596db4e65c71198b131eed0be)";

  const char genesis_hash_encoded[] =
      R"(0xd6eec26135305a8ad257a20d003357284c8aa03d0bdb2b357ab0a22371e11ef2)";

  auto seed = bip39::MnemonicToEntropyToSeed(kAssetHubMnemonic);
  ASSERT_TRUE(seed.has_value());

  auto polkadot_seed = base::span(seed.value()).first<kPolkadotSeedSize>();

  auto keypair = HDKeySr25519::GenerateFromSeed(polkadot_seed).DeriveHard(0);
  EXPECT_EQ(base::HexEncodeLower(keypair.GetPublicKey()),
            "ae70948d0c015b6c2b1ac46b8931ad6301f2c648f3f0adf71d08a68fe745561e");

  keypair.SetMockRndSeedForTesting();

  const bool transfer_all = true;

  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount));

  std::array<uint8_t, 32> genesis_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash_encoded, genesis_hash));

  std::array<uint8_t, 32> block_hash = {};
  EXPECT_TRUE(PrefixedHexStringToFixed(block_hash_encoded, block_hash));

  auto signature_payload = generate_assets_extrinsic_signature_payload(
      *testnet_metadata, sender_nonce, send_amount_bytes, transfer_all,
      recipient, asset_id, spec_version, transaction_version, block_number,
      genesis_hash, block_hash);

  auto signature = keypair.SignMessage(signature_payload);

  EXPECT_TRUE(keypair.VerifyMessage(signature, signature_payload));

  const char expected_signatured[] =
      R"(8c1e793351d610fcaf6dc13d8f3aea0c337fd0ae8fcfe7abda1ea18dce0a9a5d940dfc351961a3ffa82acd8f08f847b9c1934e2020009a4c72c6fcb4d9dd9c84)";
  EXPECT_EQ(base::HexEncodeLower(signature), expected_signatured);

  auto signed_extrinsic = make_signed_asset_transfer_extrinsic(
      *testnet_metadata, keypair.GetPublicKey(), recipient, send_amount_bytes,
      transfer_all, signature, block_number, sender_nonce, asset_id);

  auto extrinsic = base::HexEncodeLower(signed_extrinsic);

  std::string_view expected_extrinsic =
      "4502"  // SCALE-encoded length.
      "84"    // Signed, extrinsic v4.
      "00"    // Multi-address type.
      // Sender
      "ae70948d0c015b6c2b1ac46b8931ad6301f2c648f3f0adf71d08a68fe745561e"
      "01"  // Signature type (sr25519).
      // Signature.
      "8c1e793351d610fcaf6dc13d8f3aea0c337fd0ae8fcfe7abda1ea18dce0a9a5d"
      "940dfc351961a3ffa82acd8f08f847b9c1934e2020009a4c72c6fcb4d9dd9c84"
      "6502"      // Mortal era.
      "00"        // SCALE-encoded nonce.
      "00"        // Tip.
      "00"        // Aset ID for fee payment.
      "00"        // Mode.
      "3220"      // Pallet index, call index.
      "cad1eb0b"  // SCALE-encoded asset id.
      "00"        // Address type.
      // Recipient.
      "0e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e"
      "00"  // transfer_all(KeepAlive) false.
      ;

  EXPECT_EQ(extrinsic, expected_extrinsic);
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

  auto chain_metadata = MakePolkadotMetadata();

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

  auto chain_metadata = MakePolkadotMetadata();

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

  auto chain_metadata = MakePolkadotMetadata();

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

  auto chain_metadata = MakePolkadotMetadata();

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

  auto chain_metadata = MakePolkadotMetadata();

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

  // Incorrect balances(Withdraw).
  {
    std::string bad_withdraw_event_prefix = valid_events;
    auto n = bad_withdraw_event_prefix.find("00020000000508");
    bad_withdraw_event_prefix[n] = '1';
    inputs.push_back(std::move(bad_withdraw_event_prefix));
  }
  {
    std::string bad_withdraw_event_prefix = valid_events;
    auto n = bad_withdraw_event_prefix.find("00020000000508");
    bad_withdraw_event_prefix.erase(n + 5, 2);
    inputs.push_back(std::move(bad_withdraw_event_prefix));
  }

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

  // Incorrect sender in balances withdraw.
  {
    std::string bad_sender_transfer = valid_events;
    auto n = bad_sender_transfer.find(sender_hex);
    bad_sender_transfer[n] = '0';
    inputs.push_back(std::move(bad_sender_transfer));
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

  // Incorrect topics for withdrawal.
  {
    std::string needle =
        "5f139909000000000000000000000000"
        "00"
        "0002000000";

    std::string bad_fee_paid_topics = valid_events;
    auto n = bad_fee_paid_topics.find(needle);
    bad_fee_paid_topics[n + needle.size() -
                        std::string_view("0002000000").size() - 2] = '1';

    inputs.push_back(std::move(bad_fee_paid_topics));
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

  // Withdrawal doesn't match transaction fee paid.
  {
    std::string needle =
        "0508"
        "bf0be0352ca5bc12a8ac6cf0006e220e5c55bb03126890ad37ce9753f9b3e3db"
        "5f139909000000000000000000000000";

    std::string mismatched_withdrawal = valid_events;
    auto n = mismatched_withdrawal.find(needle);
    mismatched_withdrawal[n + 4 + 64 + 1] = '6';

    inputs.push_back(std::move(mismatched_withdrawal));
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
