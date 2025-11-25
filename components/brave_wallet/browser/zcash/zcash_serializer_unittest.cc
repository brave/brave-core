/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#endif

namespace brave_wallet {

TEST(ZCashSerializerTest, HashPrevouts) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                   244, 58,  55,  220, 160, 120, 126, 62,
                                   201, 246, 96,  82,  35,  213, 186, 122,
                                   224, 171, 144, 37,  183, 59,  192, 63};
    tx_input.utxo_outpoint.index = 3224808575;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {193, 161, 45,  18,  123, 87,  200, 19,
                                   137, 118, 231, 145, 1,   59,  1,   95,
                                   6,   166, 36,  245, 33,  182, 238, 4,
                                   236, 152, 8,   147, 199, 229, 224, 26};
    tx_input.utxo_outpoint.index = 1493393971;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {208, 145, 48,  246, 53,  17,  218, 84,
                                   131, 45,  233, 19,  107, 57,  244, 89,
                                   159, 90,  165, 223, 187, 69,  218, 96,
                                   205, 206, 171, 126, 239, 222, 137, 190};
    tx_input.utxo_outpoint.index = 3237475171;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  ASSERT_EQ(
      "0x7db761d908021c98a19c43f75c6486275eaca3c11f9dc6cbaf66d3050c23b515",
      ToHex(ZCashSerializer::HashPrevouts(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashOutputs) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);

  {
    ZCashTransaction::TxOutput tx_output;
    base::HexStringToBytes("630063ac", &tx_output.script_pubkey);
    tx_output.amount = 1264123119664452;
    zcash_transaciton.transparent_part().outputs.push_back(
        std::move(tx_output));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    base::HexStringToBytes("636a5351520065ac65", &tx_output.script_pubkey);
    tx_output.amount = 810835337737746;
    zcash_transaciton.transparent_part().outputs.push_back(
        std::move(tx_output));
  }

  ASSERT_EQ(
      "0x0dc9291fc891c10bdecedde449fa319cfa3f45cf7779423c2272c013d7fe0080",
      ToHex(ZCashSerializer::HashOutputs(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashSequences) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 1290119100;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 3797894359;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 4015866081;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  ASSERT_EQ(
      "0x17cae6cde4962f6eb86b350eb5a80d5576a958b4bd3438689e94ee387eb80f8e",
      ToHex(ZCashSerializer::HashSequences(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashHeader) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);
  zcash_transaciton.set_expiry_height(10000);
  zcash_transaciton.set_locktime(1);
  EXPECT_EQ(
      "0xc632e4b84e69afe329c646d3eaa71935a8922f8f2236ba3603c439bdb939db83",
      ToHex(ZCashSerializer::HashHeader(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashTxIn) {
  {
    ZCashTransaction::TxInput tx_input;

    tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                   244, 58,  55,  220, 160, 120, 126, 62,
                                   201, 246, 96,  82,  35,  213, 186, 122,
                                   224, 171, 144, 37,  183, 59,  192, 63};
    tx_input.utxo_outpoint.index = 3224808575;

    tx_input.utxo_value = 1848924248978091;
    tx_input.n_sequence = 1290119100;
    base::HexStringToBytes("ac0000", &tx_input.script_pub_key);

    ASSERT_EQ(
        "0xb39969f0fba708491e480d80d4d675a1f1552cc7d479d7942f75fa31ad9c6ad6",
        ToHex(ZCashSerializer::HashTxIn(tx_input)));
  }
}

// https://zcashblockexplorer.com/transactions/360d056309669faf0d7937f41581418be5e46b04e2cea0a7b14261d7bff1d825/raw
TEST(ZCashSerializerTest, TxId_TransparentOnly) {
  ZCashTransaction tx;
  tx.set_consensus_brach_id(0xc2d6d0b4);
  tx.set_expiry_height(2283846);
  tx.set_locktime(2283826);

  {
    ZCashTransaction::TxInput tx_input;

    std::vector<uint8_t> vec;
    base::HexStringToBytes(
        "be9ef0f2091d0ef49f7f32c57ec826877175e9a703bef5989261e42bdfd69171",
        &vec);
    std::reverse(vec.begin(), vec.end());
    std::copy_n(vec.begin(), 32, tx_input.utxo_outpoint.txid.begin());
    tx_input.utxo_outpoint.index = 1;

    tx_input.n_sequence = 4294967295;
    tx_input.utxo_value = 751000;
    tx_input.utxo_address = "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi";
    tx_input.script_pub_key =
        ZCashAddressToScriptPubkey("t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi", false)
            .value();

    tx.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
    tx_output.amount = 100000;
    tx_output.script_pubkey =
        ZCashAddressToScriptPubkey("t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi", false)
            .value();

    tx.transparent_part().outputs.push_back(std::move(tx_output));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi";
    tx_output.amount = 649000;
    tx_output.script_pubkey =
        ZCashAddressToScriptPubkey("t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi", false)
            .value();

    tx.transparent_part().outputs.push_back(std::move(tx_output));
  }

  auto tx_id = ZCashSerializer::CalculateTxIdDigest(tx);

  ASSERT_EQ(
      ToHex(tx_id),
      "0x360d056309669faf0d7937f41581418be5e46b04e2cea0a7b14261d7bff1d825");
}

#if BUILDFLAG(ENABLE_ORCHARD)

namespace {
void AppendMerklePath(OrchardNoteWitness& witness, const std::string& hex) {
  OrchardMerkleHash hash;
  base::span(hash).copy_from(*PrefixedHexStringToBytes(hex));
  witness.merkle_path.push_back(hash);
}
}  // namespace

// https://blockexplorer.one/zcash/testnet/tx/496dfffff625ada462cbc8a733f305fdef1ca584ceb8e7efa5e28e38249b466e
TEST(ZCashSerializerTest, OrchardToTransparentBundle) {
  OrchardBundleManager::OverrideRandomSeedForTesting(6675565u);

  ZCashKeyring keyring(
      PrefixedHexStringToBytes(
          "0xe2c0aa2746fc727734c3beec18493053ca3e624fc6d4c4bffbcd7d56ae0f12c864"
          "70226552ba119ecb4de091bf51bc77ba22e1bd264af84ff5da575029edeab9")
          .value(),
      mojom::KeyringId::kZCashTestnet);

  keyring.AddNewHDAccount(0u);
  ZCashTransaction tx;
  tx.set_consensus_brach_id(1307332080);
  tx.set_expiry_height(3667223u);
  tx.set_locktime(3667203u);

  // First input.
  {
    ZCashTransaction::OrchardInput input;
    input.note.amount = 1000000000u;
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0xc140e946097edb3cc19d0b341569c29882ca9c6e2ca54b7e0fc31eb41691f007830b"
        "5f240f0f02d7cac925",
        input.note.addr));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x8636f0a41dab6e82c23c7277de1fbcf7d69acb0cce21ffd29a08e20d851de63d",
        input.note.nullifier));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x9c26b138ce792a30cdb15a6e48351b2a1a563328bc2c62395959e226f6afc000",
        input.note.rho));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x479e400000000000489e400000000000499e4000000000004a9e400000000000",
        input.note.seed));

    input.note.block_id = 3667070u;
    input.note.orchard_commitment_tree_position = 114562u;
    input.witness = OrchardNoteWitness();

    input.witness->position = 114562u;

    AppendMerklePath(
        input.witness.value(),
        "0x0d43a87040b3fdbd5f93e90f9b108ae147a4e9f9355bdb580c0f6c255510e533");
    AppendMerklePath(
        input.witness.value(),
        "0x6e66f421fe8dc73ec4ea74b0b8d1f0c224418cf3ce7f71a2271be1802ae26d03");
    AppendMerklePath(
        input.witness.value(),
        "0x5b3361735c9e21afa62ba51d01dc6d3371b076600b8e57cd62ab5900e449290e");
    AppendMerklePath(
        input.witness.value(),
        "0x2111fc397753e5fd50ec74816df27d6ada7ed2a9ac3816aab2573c8fac794204");
    AppendMerklePath(
        input.witness.value(),
        "0x806afbfeb45c64d4f2384c51eff30764b84599ae56a7ab3d4a46d9ce3aeab431");
    AppendMerklePath(
        input.witness.value(),
        "0x873e4157f2c0f0c645e899360069fcc9d2ed9bc11bf59827af0230ed52edab18");
    AppendMerklePath(
        input.witness.value(),
        "0x27ab1320953ae1ad70c8c15a1253a0a86fbc8a0aa36a84207293f8a495ffc402");
    AppendMerklePath(
        input.witness.value(),
        "0x86c7a8020a25e94fa40b606eab684131bd9a9593f6f27b615cdd55e56b423d18");
    AppendMerklePath(
        input.witness.value(),
        "0xff003576721efedc9ec62db4bcba91fc1a0738b18cac5c15eb316f77c5243826");
    AppendMerklePath(
        input.witness.value(),
        "0x9e1bfb94f5dc4b7c15092ad23d89c4e3f197db541f54b3587f404c1d4d981b0a");
    AppendMerklePath(
        input.witness.value(),
        "0xbaddc63339bcecc8faed385b6f16576c6378d9cae9ec60974265b4afc1781f05");
    AppendMerklePath(
        input.witness.value(),
        "0x73ae007fce7670a9f40639ef998b23b8e8b7990c9894b13de595d7592f32c223");
    AppendMerklePath(
        input.witness.value(),
        "0x622c7605d17857460d412e4fff3265e15416029f607b7a5daaf2511a40201511");
    AppendMerklePath(
        input.witness.value(),
        "0x067fc882d2ebfdf7969fc02814cbe49dda32e267e963a0e79fad8eb7e0694b04");
    AppendMerklePath(
        input.witness.value(),
        "0x3f98adbe364f148b0cc2042cafc6be1166fae39090ab4b354bfb6217b964453b");
    AppendMerklePath(
        input.witness.value(),
        "0x7f93215cf466b333690a951c86254d6729672189dc06160f378267efb70b580a");
    AppendMerklePath(
        input.witness.value(),
        "0x25934a8c8cde7b4ba7e51d78f2321c7e286d140811a192f692f29d3f0ecce510");
    AppendMerklePath(
        input.witness.value(),
        "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a2578b2738a6d331");
    AppendMerklePath(
        input.witness.value(),
        "0xca2ced953b7fb95e3ba986333da9e69cd355223c929731094b6c2174c7638d2e");
    AppendMerklePath(
        input.witness.value(),
        "0x55354b96b56f9e45aae1e0094d71ee248dabf668117778bdc3c19ca5331a4e1a");
    AppendMerklePath(
        input.witness.value(),
        "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909e72bb78d33310f705");
    AppendMerklePath(
        input.witness.value(),
        "0xe81d6821ff813bd410867a3f22e8e5cb7ac5599a610af5c354eb392877362e01");
    AppendMerklePath(
        input.witness.value(),
        "0x157de8567f7c4996b8c4fdc94938fd808c3b2a5ccb79d1a63858adaa9a6dd824");
    AppendMerklePath(
        input.witness.value(),
        "0xfe1fce51cd6120c12c124695c4f98b275918fceae6eb209873ed73fe73775d0b");
    AppendMerklePath(
        input.witness.value(),
        "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b3335a0aaeb63a0a2d");
    AppendMerklePath(
        input.witness.value(),
        "0x5dec15f52af17da3931396183cbbbfbea7ed950714540aec06c645c754975522");
    AppendMerklePath(
        input.witness.value(),
        "0xe8ae2ad91d463bab75ee941d33cc5817b613c63cda943a4c07f600591b088a25");
    AppendMerklePath(
        input.witness.value(),
        "0xd53fdee371cef596766823f4a518a583b1158243afe89700f0da76da46d0060f");
    AppendMerklePath(
        input.witness.value(),
        "0x15d2444cefe7914c9a61e829c730eceb216288fee825f6b3b6298f6f6b6bd62e");
    AppendMerklePath(
        input.witness.value(),
        "0x4c57a617a0aa10ea7a83aa6b6b0ed685b6a3d9e5b8fd14f56cdc18021b12253f");
    AppendMerklePath(
        input.witness.value(),
        "0x3fd4915c19bd831a7920be55d969b2ac23359e2559da77de2373f06ca014ba27");
    AppendMerklePath(
        input.witness.value(),
        "0x87d063cd07ee4944222b7762840eb94c688bec743fa8bdf7715c8fe29f104c2a");

    tx.orchard_part().inputs.push_back(std::move(input));
  }

  // Second input.
  {
    ZCashTransaction::OrchardInput input;
    input.note.amount = 4119955000u;
    input.note.block_id = 3650412u;
    input.note.orchard_commitment_tree_position = 114351u;

    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0xc140e946097edb3cc19d0b341569c29882ca9c6e2ca54b7e0fc31eb41691f007830b"
        "5f240f0f02d7cac925",
        input.note.addr));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0xc98823d9a80fa5b24c1945bef5ccfa6629ef3ef93dc367efa51a9b19269cb000",
        input.note.nullifier));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x489b326aaf4915fd4ec12a69deef99b4833305ff7709589dc0b0a6d2ae37ad29",
        input.note.rho));
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0x405d8d5c8e1a72b9ceccb92ccac989296e4347f75d7e2fb8dab9dc0813338cc2",
        input.note.seed));
    input.witness = OrchardNoteWitness();

    input.witness->position = 114351u;
    AppendMerklePath(
        input.witness.value(),
        "0x128f444f186d6b83829b57cd8c77cd322f9bfa49f064bbd1d5446376ee44783c");
    AppendMerklePath(
        input.witness.value(),
        "0x0920e64113da2b8f5c7315793b45c4565c3b94132f89672e5aed00f1f2457210");
    AppendMerklePath(
        input.witness.value(),
        "0x1787618701339c8dd9140cd8e1c40b4e3282d948462508ff84b99da165a07806");
    AppendMerklePath(
        input.witness.value(),
        "0xc5f99f1f55bb5be2324301827b5f7f1a2c58db6d5a5af584e8633c1f6b121727");
    AppendMerklePath(
        input.witness.value(),
        "0x9972b85cf445530d432e66a4ad3a780a2859f8a6f54c1f4268f5a6bea0c8bf29");
    AppendMerklePath(
        input.witness.value(),
        "0xb33b32a4d69abd2d17b830b17e965a99444e8e5f28c48a1a7e59afa747087e03");
    AppendMerklePath(
        input.witness.value(),
        "0xa39c534e2b4c2f0d5e670326c6695015a7ad2e223dd3552222ea27a55fca7e10");
    AppendMerklePath(
        input.witness.value(),
        "0x7bb74adcadc0aeee4333c9303b6b4ca42761f510cd430168bcce42b0cc806437");
    AppendMerklePath(
        input.witness.value(),
        "0x816b2aa9515db4160db45b189f1d1dbcba5a95c10996fe86400a082aecc7bc31");
    AppendMerklePath(
        input.witness.value(),
        "0x9e1bfb94f5dc4b7c15092ad23d89c4e3f197db541f54b3587f404c1d4d981b0a");
    AppendMerklePath(
        input.witness.value(),
        "0xbaddc63339bcecc8faed385b6f16576c6378d9cae9ec60974265b4afc1781f05");
    AppendMerklePath(
        input.witness.value(),
        "0x73ae007fce7670a9f40639ef998b23b8e8b7990c9894b13de595d7592f32c223");
    AppendMerklePath(
        input.witness.value(),
        "0x622c7605d17857460d412e4fff3265e15416029f607b7a5daaf2511a40201511");
    AppendMerklePath(
        input.witness.value(),
        "0x067fc882d2ebfdf7969fc02814cbe49dda32e267e963a0e79fad8eb7e0694b04");
    AppendMerklePath(
        input.witness.value(),
        "0x3f98adbe364f148b0cc2042cafc6be1166fae39090ab4b354bfb6217b964453b");
    AppendMerklePath(
        input.witness.value(),
        "0x7f93215cf466b333690a951c86254d6729672189dc06160f378267efb70b580a");
    AppendMerklePath(
        input.witness.value(),
        "0x25934a8c8cde7b4ba7e51d78f2321c7e286d140811a192f692f29d3f0ecce510");
    AppendMerklePath(
        input.witness.value(),
        "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a2578b2738a6d331");
    AppendMerklePath(
        input.witness.value(),
        "0xca2ced953b7fb95e3ba986333da9e69cd355223c929731094b6c2174c7638d2e");
    AppendMerklePath(
        input.witness.value(),
        "0x55354b96b56f9e45aae1e0094d71ee248dabf668117778bdc3c19ca5331a4e1a");
    AppendMerklePath(
        input.witness.value(),
        "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909e72bb78d33310f705");
    AppendMerklePath(
        input.witness.value(),
        "0xe81d6821ff813bd410867a3f22e8e5cb7ac5599a610af5c354eb392877362e01");
    AppendMerklePath(
        input.witness.value(),
        "0x157de8567f7c4996b8c4fdc94938fd808c3b2a5ccb79d1a63858adaa9a6dd824");
    AppendMerklePath(
        input.witness.value(),
        "0xfe1fce51cd6120c12c124695c4f98b275918fceae6eb209873ed73fe73775d0b");
    AppendMerklePath(
        input.witness.value(),
        "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b3335a0aaeb63a0a2d");
    AppendMerklePath(
        input.witness.value(),
        "0x5dec15f52af17da3931396183cbbbfbea7ed950714540aec06c645c754975522");
    AppendMerklePath(
        input.witness.value(),
        "0xe8ae2ad91d463bab75ee941d33cc5817b613c63cda943a4c07f600591b088a25");
    AppendMerklePath(
        input.witness.value(),
        "0xd53fdee371cef596766823f4a518a583b1158243afe89700f0da76da46d0060f");
    AppendMerklePath(
        input.witness.value(),
        "0x15d2444cefe7914c9a61e829c730eceb216288fee825f6b3b6298f6f6b6bd62e");
    AppendMerklePath(
        input.witness.value(),
        "0x4c57a617a0aa10ea7a83aa6b6b0ed685b6a3d9e5b8fd14f56cdc18021b12253f");
    AppendMerklePath(
        input.witness.value(),
        "0x3fd4915c19bd831a7920be55d969b2ac23359e2559da77de2373f06ca014ba27");
    AppendMerklePath(
        input.witness.value(),
        "0x87d063cd07ee4944222b7762840eb94c688bec743fa8bdf7715c8fe29f104c2a");

    tx.orchard_part().inputs.push_back(std::move(input));
  }

  tx.orchard_part().anchor_block_height = 3667180u;

  // Change.
  {
    ZCashTransaction::OrchardOutput output;
    output.value = 119940000u;
    // Change address.
    output.addr =
        keyring.GetOrchardRawBytes(*(mojom::ZCashKeyId::New(0u, 1u, 0u)))
            .value();
    tx.orchard_part().outputs.push_back(std::move(output));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "tmFDtouv8PyCYZjky6Y4z7Y67fXKka82bA9";
    tx_output.amount = 5000000000u;
    tx_output.script_pubkey =
        ZCashAddressToScriptPubkey("tmFDtouv8PyCYZjky6Y4z7Y67fXKka82bA9", true);
    tx.transparent_part().outputs.push_back(std::move(tx_output));
  }

  OrchardSpendsBundle spends_bundle;
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashTestnet,
                                            mojom::AccountKind::kDerived, 0);
  spends_bundle.fvk = keyring.GetOrchardFullViewKey(0u).value();
  spends_bundle.sk = keyring.GetOrchardSpendingKey(0u).value();
  for (const auto& input : tx.orchard_part().inputs) {
    spends_bundle.inputs.push_back(input);
  }

  std::vector<OrchardOutput> outputs;
  for (const auto& output : tx.orchard_part().outputs) {
    outputs.push_back(output);
  }

  auto state_tree_bytes = PrefixedHexStringToBytes(
      "0x0125caefe74aac74b69f3ce98b895ab2908f9abf6cf2bbae77a490024ca9b6941a001f"
      "0178c62a5059a7b7a40d24197299959b43d65ea775c2e6f8d82eb35fb27569b80801ccfb"
      "c15aa12de3be6eb2ff74819788ac63c2adf3007bff1ea4ee7d4c3beb100a000000000186"
      "c7a8020a25e94fa40b606eab684131bd9a9593f6f27b615cdd55e56b423d1801ff003576"
      "721efedc9ec62db4bcba91fc1a0738b18cac5c15eb316f77c5243826019e1bfb94f5dc4b"
      "7c15092ad23d89c4e3f197db541f54b3587f404c1d4d981b0a01baddc63339bcecc8faed"
      "385b6f16576c6378d9cae9ec60974265b4afc1781f050173ae007fce7670a9f40639ef99"
      "8b23b8e8b7990c9894b13de595d7592f32c22301622c7605d17857460d412e4fff3265e1"
      "5416029f607b7a5daaf2511a4020151101067fc882d2ebfdf7969fc02814cbe49dda32e2"
      "67e963a0e79fad8eb7e0694b0400017f93215cf466b333690a951c86254d6729672189dc"
      "06160f378267efb70b580a0125934a8c8cde7b4ba7e51d78f2321c7e286d140811a192f6"
      "92f29d3f0ecce510000000000000000000000000000000");
  auto orchard_bundle_manager = OrchardBundleManager::Create(
      state_tree_bytes.value(), std::move(spends_bundle), std::move(outputs));
  EXPECT_TRUE(orchard_bundle_manager);

  tx.orchard_part().digest = orchard_bundle_manager->GetOrchardDigest();

  auto shielded_sighash =
      ZCashSerializer::CalculateSignatureDigest(tx, std::nullopt);

  EXPECT_EQ(
      "0x6e469b24388ee2a5efe7b8ce84a51ceffd05f333a7c8cb62a4ad25f6ffff6d49",
      ToHex(shielded_sighash));

  auto orchard_raw_part = tx.orchard_part().raw_tx =
      orchard_bundle_manager->ApplySignature(shielded_sighash)->GetRawTxBytes();

  EXPECT_EQ(
      ToHex(tx.orchard_part().raw_tx.value()),
      "0x0256fca85195acfc96859a47b2cff57aeb6eb71c84f60d2c5bdc37243baf8847a2c988"
      "23d9a80fa5b24c1945bef5ccfa6629ef3ef93dc367efa51a9b19269cb00045e6d46d7fde"
      "fd518b0c3b205d09439f044cba4c24aa3e8172ddb7647a0491367988a8f6d8f0b269fd17"
      "3314f8256eb50e015756c1c7596c92f4daa956e7a31e8ec61d7d750c91cc8700a34c9edb"
      "975103c0bf06d7c4c29b1851739b1d865099c7a5aed5374f0ce22b76d136a80b4bda1354"
      "dc1811c7211ce6c98718420f72d3f428e869a31b445e3ab0816c79c7661f7cc8bcd97599"
      "87b61a33d28b5748900fa55d5c13925e45a03bcc7111bbb8c9f688e35f9fa888e1e044a3"
      "d34d9004b7f19ea2bc1735488047d8c60d7522e541ea0a344d9d6166030da69232c0f6ff"
      "96ec9de74b50e679d4afa624529e7d9439db4045513f34777de67dd7a4f7a73605ebf85f"
      "4240e520ef62fa21ef1e05fa187e1e594bb875e806501c2fdcc5a85c087b0cfa88271371"
      "a365151a429ba383ec37ee7ae786a66d612518851b2b6d085f0564fa2a75517d648212e3"
      "4d9dc3dca2a07985627beb4a4883b637d0609de6f4b09c4cb0ec2a3405dc0b41fcd1cdad"
      "d517daa992d22c57d751162306fba9bd1d700c55d6808e0a5bb5c0008e83910b53011afe"
      "4e4d349841b50e9c943361584628cd9e3492210e29bca3fc89efd931f2b54d5c775c8751"
      "7075b34a205bb170ea572dfb1ebd6fc03baee93254b43f1f87d8bd722dc2bdf4f5763815"
      "dd476c2eb4d7c72b8f0816a6af0e35d5e1f61da3c0da19656a1ea0b1ad26221093971fe6"
      "6fa7d9d3ab4a49f562213b30c3c8fb8cc94026eb7bf42c937329441f666d40db5016c36c"
      "9d367f53f7b6c56c6334829e8f2d501ede1efa0233d4ffb1ad1cb1b1bd1d0eb6270d31da"
      "e521d702ed135606f7a7abffa294ba1eb472694bdb7598b0c338e063f2065ebe08da8b0e"
      "b09a02760cb5ab93641289c0825c7bd29b2e33ff8d1b176a1ad9c785d33809877329d511"
      "773af7db831bb5d4ce44176968f15bb37b86faf386948d1b5cfd996daca41709d4fc217d"
      "fe1a03f08a76d89a4bd36f7e7a1a6dc4bfe6432cbbc127f219d7711225919ef2a98188e7"
      "e1b268d68d76168819a7e40ef0d9fcc75aafbf57e7f70f3972d228a69c6e4d127a67733b"
      "0f11caea37185562822cffc1502bdb1a044e9cb7bdd30c1bf49b8636f0a41dab6e82c23c"
      "7277de1fbcf7d69acb0cce21ffd29a08e20d851de63de3b7d5fd9c1b0ea21f208891905e"
      "706afc79e13008acc5625adadb94cefa913974699c691b5dd78162d0b96ae32d9755f029"
      "de1ad4af996fd91cedad3b96df37553aa9aafee8053ddbf14e4473aa80b2b46d0f1f6efe"
      "dd849db82f7c5b9dd1ab3ba4c0fc73bb3854ae3ed140932cbfa1e3ac479da9e5178a1a0c"
      "9388e360b6352a8d793350ba0043d70f769a8c6af945974ba0a67a905d8ba86061eb25e9"
      "a1f53313666cbad49a930493e1a37f743fd904ad56035e58979ef2311d4349fc3784bb21"
      "e7690efff17fc240d801dac1b8b29af646c57a3858428aff61422f61d6bdfa8d7bfd55d7"
      "0302d1776b5757f33b5c1dcf4a35b0a9e43c40917ec7fbb97a971d3effc85b008ff5505c"
      "10d6c2209057840f570a25d74b1f0074b727a92b6c94b792a447285ec476160d3f47b160"
      "159fd1d327d8382af138641225feff56bab425aa93c734a5a7420ef63ce75c0d45af42fb"
      "b9810ce15e25b7abf519c6717e437306631ff39a9f7d85a5c38d1d0c8cf5f9ff90d2fd14"
      "e03c6a4aa8a966a8e7245e6968a7708a6025ae2fc049516f22113f113e83f0435dd18c29"
      "48efed5816f13b81995f7ca441b40cbb4e121faed3ebdae91c70707c8b7079e12e3721c5"
      "9300495348f91692016de289b606d5506b8a52f1306780e80ce2e61a91d45dcecc302ece"
      "940b4a166623a93e33ad5dad377165cfb5e2d1e2c6c599bfea546baf2481a1db87bdcbe0"
      "84047f590b9e48ec2f3a955a29e632a893c02f65c8016d61225c289064f868bcaf9f69bd"
      "3650e883f259b83fa1c25900d653791add8743f5a77e24bddabcaa716e2254d29dcc73b7"
      "196c1e9568f2aa873e99bb9b1e75b147220ed68918fa4e2c4032bccbc2025763d8578aeb"
      "acf8b4096cd31002e6ac4e9437c068b065709780bf05f398e7e5526414c10214216dec25"
      "e81022b976b608d413776dfd2d74496ac90162f9cd101f8c41dcacc31f0db6d063de96c7"
      "1504a014ad40ca6be2bfe8e68a16ae4160f5593e360ccf2f5acc02263d67ccac8045bd23"
      "5c6662a95fdb9b6b5c5b574a36f6a4ae8412ba3a6b5503982c062a010000008dd74f0366"
      "1630ab41e830ee088e7d63f587774dc9c515d8773213014023f001fd601cd187a948ad21"
      "a4838894ecafefc0ace7eb82a007a253c92c3ed67b248a2a0c00cea086bbdf94ddc4123b"
      "246c308d8a25148403c1fd9f757ea0e3804a7e746f8e89f1be9e7d17ab87bb42167592a6"
      "6d9e497773a5acffe244350c06c40c40d535fa3d6b580076d7be77c1d60f5cab41411f79"
      "215832130d73242c737c9d3f92b38debf68f9405a517efa456c0a2fa727ff7bdd170f014"
      "fb925ed737056920e182a6eda5db3d45885e3a309859804455f671373ffa01c321cb5ec3"
      "2fe4674ee60f1f1deef2d6e1d3ab51eec2287e8d602f0c9a6db75ede22d88c344bf3d67b"
      "ae85738b61372f6eb3e9e9029543b252d107dbd9e10295df4d949f28121f1431c1ba349f"
      "a326d3abe266cbbeba868e7ed4ff2a41e9f005d85a094ab7bd5bdb8f06b81973defbb66a"
      "0a05bbfbddd48fb0c1702e42aecca8d98589bf19167e13eae2af30b0f647ae3357898ff9"
      "1e727d5f393b3240ce5a28c438c7cfefac30dff945174426f44bd7894d059e6475c8d079"
      "c56c944b5f6cef31f6e288133e4c946e8b21e35c61ad952e9f7bd2c60b2ab56c59fe2c35"
      "2955e0bdfbb4823e985295abb184b9ba796297ae21334627240aa7df536414003c6e2a15"
      "08139354e8ea59ad3ab0046f876f03fda1c1b22f9faa1d2959477e9d9194e55b7a993d0d"
      "3d4740d8b6baeb703d3f863e965dd7582e52cae8e2792c595cd99fa4cea6f888bbd6a0d3"
      "6d8c008d30f0ef39e9b142884756bd6f223c99b439cee2442bdb60d60333b9f423a9d86a"
      "59e619cc2729f1ec56512b9e13f661a4f8de2f8a9b05a3eb2ff18abd9ca6b7376914a38b"
      "00f1eb1e099b35e0daafa782fe5b405b6cab9cafedaf99149f97b7d02da9b3d2d24d9dd6"
      "16902baa3376669f953b3be03aedc52c7c38d0cb33a14042fde5d737569dffb02029ce04"
      "460045676c71faf7629587981649f80a4b11434a08165f8f54485aa6d10543a01a6a53ff"
      "86516fb47eac61e78ac3d1b29caa350b8aac7c1292c9448c9e06184dedd00b2d213c30ff"
      "87d4e147e848867e21b2ded38e85ca76d2d5d4b8a7eb5cbf277d0d6eba7c112e395be04d"
      "1484de8f93191e631d62baeaee8391794031aa0abecbd2f1a377deaa1a3487736e4d8098"
      "a6b6c88258a57154120ba2b1f49d84b4fb864e1961dfc9b016ff523334d206744c0a7cf0"
      "11e7bbcef005cd08b5bb21c6ef7c511d7d9bcb8aa5c5b2a6afeb3e86570126976a32c652"
      "90b8e04f60f91b5b8fb1839d99bc08171aa731da67da3e837a072a526fb9212c7f13b3c1"
      "c2bdcef8e53f2d5d52a1505eafa3a474a32e83a83d04c20acf7c710e06601f0d445b38d3"
      "5f8a464d08f41269ed44306789f538895833b2a487d6efa6b80a7dad08a45c68a5cdb7bd"
      "38e79b00c0d3d44fca20ea04a3a9935ea11da665655e6189643aa9ebb49829aedcb33f44"
      "46ff33ec0c8bd3c0b82b4533e024386a11ff2f6392e89ef54e5728360b2f8f54d40fb397"
      "0900e3a9b53a590033bbb69434ee1a4763d57580429b158c35d3aa32924a5270937fdaf4"
      "13a36e3bae78b866fa9f8628fbc8820aa5490dc54f55cb88869ce07807cc7cef74269f74"
      "841a5a57cab6623ffddec217e1433e3f3e8857f192fed479aaadb3f9aa3fdc2aa8d6c4cc"
      "7fc4f218ea598a904707b216c3007995b17dcfecffe06f8ab8b46033143cf3aaf9739d48"
      "3a7049dcbb2cf20e5a3949a21daac8412af5e62c63195fed0b77f050260652055c0647a9"
      "c620556b0ae7b20fc43372506a6527d6d5903cae4c4b6a8a9db368402c0f475f6021ce8d"
      "98f085ad632de654a41e0e074801ee957dff3cacba5ced0850369c40abb0efcd9096be77"
      "2fb8cd497c5e0372db1249aa309bfc579d09b3bacb9ab95a055652ceec085a1001764f72"
      "a97721cc668ae73302d8f14d97ea1cac28b03ebde8acf12c5dd21364f5aa1cbdd884cf31"
      "018cdebf5c5a9688a7dac4af920509c826c11096e310c56d00995705963081a8ac994b55"
      "4690badb1264eb589c4a30c5f2602fdb0da081bc02c460a4a4e7d7bdb09ca109bf37ced9"
      "7f39072e6ea579e138a8b7040c2affb6a6221d4854af26481527ec0e9e13d002f71581c3"
      "d421c9d7fcc23650a8f0abdac812b16e1dc430fe029a9ba8901fa79bb55b318023d5fd99"
      "a2953b6aa363c2039ca7434fb56abadb0d2c8e9fc2f13f5e60991cb3733a1267bd59b93b"
      "56a3ed33983742c4de57cbc26a1f725b2c61bae179332cd58dda30f1f5b3ed73ec2cef89"
      "8fe8c9417360b6f5f5252561cef03c2fe9c74c5cbf6cd30b72a718c021317bd0240810bc"
      "7e1c754dffab00a2d767922b25ee15747a0d845de4d0c88f46b86397ac46d8af79228115"
      "eb8b89f4da69e32bf3ea53d7bc5a6d34ae43ad86ade87b8cba8bc6c94ae96bb4c336d4fd"
      "94e5be1aab6ead30c7ae513477ba118462845abbe0cd878dfe6d1d73bf0adf020e7f423e"
      "e37c4fe0035d4047ab6b2a4ec07cb3cbc620bac9051e466d32130cd4372a64cff887bc54"
      "1f555799bb6cbe6ac2c5d217ff5ca0ac06dd452e8e009c109921077db5421b52ea0631ef"
      "1519b15a97b86af22487bb7c46b2960a60017ca743bfc2184e291d49195d8305a1d5d0c1"
      "fd959385a74046ce4dca6ca6152e165baf161e533be476be6d2466a1fb213b9f6fddb2de"
      "d5a74dffee95735c651944bc93f60a86c21b330514bc5038924126d54ddcd9a8fefdd1fc"
      "047f4c7c673cb517b13f56d8332cc741dff5e893146fe70c5fbc4825e8bd0ecbbe847668"
      "27134e552841709f2517c195619fb3c3cef82cb321cf55459cff75e92d01acbe5631519f"
      "18348df919e3161b99947ecb202305b8332359437bcb63e229cc8b5e9c0c90d7762226a7"
      "a36fd58024e07985cb824558ec6b7520690d9e2f65a341b48809c3e489c17b8286185d1e"
      "a542bdb863c8facfa9a8f5448077b468e690c604243a252df3da7e7e1479c19262d0ae9f"
      "6cc36133e3f680ce1ee13c9c7a4f312cf813cf430bc00e30b252bb73e3a6275392d0dd72"
      "1d37ed71cb485fbbd9618b524527843709aa6bd4c25480fde0bcf9919f44e4e12098317f"
      "20634a026046040840265b11c36e316233ca7df8688b7fc957918e524503766551c24329"
      "5daea36718314fd65d29dc1bb0db2b51a7c01a632ea12281a8ad9970982abbf3e2a6b111"
      "9e1742beb37fb001203c75c54bc3802d78e770a87397ecf57c10a4089789f4bccb1189c2"
      "57a51467344cbaa4692da1374ebc49ac8293c2a23c6e40cf6d76966f4c2efefd929a74bc"
      "bfe71bb5d8b0a517c2b0823b1606259cff5c21477ec71ea1b00b1f2bba2756569b550af5"
      "3ecfc399824feb769a90b96ba3e1c4e6afdb5b8b64392d150ae5adf7f8a1793b8dd87420"
      "fe0cf95fb8e64a06f1b2ede3161d7e80253909ff05d1daa9e5cfca07a8f06f64880e3b17"
      "47f7755489d0f2f80607566c4d0b065b0a2317718307865f6e548b63ae09c316f2d833db"
      "e5ea943f820848c44926e6710f8c6a0c023af9f45670728db1aaa1966084fe780f7b8681"
      "9286fac07f1f4c181a6f8e7d4e887efaf78efe460c7e64f34e6ee9c6f0ece91cb57169db"
      "5b093d7e37c511cdc811d48c3c6e5304f269157216b3db870302eea61eb31e2b300829f6"
      "2c25e49ee5805dfb240d07a4223f0578c9d7905d143b893e629ee0f34f15e1b9c5f779e1"
      "692d3acaf0482f72c41d4cdcfc5bc7f76a8429482aebd4c3f02c0757fdf4752dc9f7b472"
      "c2cb550c45afbb4a639cfc0b21d7c7a74bb22752fd04848c86d149c36fce4fb5ec159257"
      "a467b1aeefaf87a7328e4032a194a2390d1117d1185383b0dc7ba3e9e899c16e893a40e5"
      "051be88a84bdaeb1aa8e3337b82e36acfffe989a1556049c1ee14ec710d3d5eb0fe4af3c"
      "573498e7aad3de7f8d25639f3ddc3daeef2dc48686be40aa575711041351808ffa5220dd"
      "3b2f6a49f936a1b4a3b7e3897dc296e65b4295bfb00d1bafda26038d540c9a06cc7682b0"
      "c11c428dad7493ad80fe30319b021171f8e80982ecf80edae8a35269627dc021582d005e"
      "1dad64848419ea2a47903e76d9f480caeb5d7810c09a397be69a15dad53b54a41bf97209"
      "242762d34330a4fe01cd11476b0cf950979cbb0fd4efc0c6da0461bfba874996b2733b8d"
      "d7eea8384792403db4d7f742e19ffca2f6cc07d5f83da004985a9af389855cd05a295cac"
      "6de29e434936c3c99bc3d58c5e8d7793b4195113bea73ebe48af62876accbf5d76e6ec15"
      "17c6b5325542421abd9ffae67f002bc87af7a8eec9bab731694a641f74ff3397dda42d85"
      "984406857775ed018c12dbd8639100f04766b3b39d24eb399de67e66d18c83c88e346aa2"
      "594ccd8eb91f6c5333c35e529013e994884d30dcd229b3d4fe8fcb3a38d3fb2327def48b"
      "102d7571cb07cfe798fec9e28972ec3ce4c1eb6f84f94b6a13b2649b5ece6abb2f29d683"
      "c8bdb38adfe9a4a4520413bd191476286094d47b2627ff98287225ac09117ed1e4852ea1"
      "da0ab0eb7216f7b1d608c27be33c0225b44e6b3cc37edafe9f2e7ee050c1fbcba913e4ad"
      "fa4c2c6250ff9699642703405f9d3cae85d995fa58182d4333962e0ad8d177e46d25e94b"
      "1701236e4bd8b0b62bb4548d8becf6c9de1d572d893937fca52ad2efc5c11743acd45391"
      "02a73aae74544006d87d4cdd2227ac24486fc6585ada3d04a917fc84f8fe77937828c202"
      "9cf4653eb996621eb514e18d81b66a85136ad5f6ba0dddd170cee50881c0c9f6260f0978"
      "6a601518bb3c46afe03a0ffbbe4cba777c0dac10471cdf1e9ea8c5d9f97765ec724f90a0"
      "3e30fd04dc0edc3d4cad76107bd30043971cee3c149be8556dbc0e081ca1fc318c3f5616"
      "58ea320f6ef68cc57e1d867853a1c992669d19baa2ccc26e0a18320f25294a59a73e4014"
      "7d56e9fc136aef6df6fd3328732b34548ca5c582bd596265db294830e9054f0e217123f8"
      "920c4b9e066945d97707fe4845cbf2380bae7b64663a17bf9773a5abf49f2d9884b2f2d3"
      "d71529a19a0cdf13ba1ff37ce5a86801ba3a30f474d7c6a122b1bcf7ee74e6e413facf89"
      "e20bb469c6a0f2bdd67d34c6883ad1f9969d3378ca3087e0bc7aac32095dd1c6fb8ec6f7"
      "68a07ebfdc72aedac3080dbd0459c18ac8abfd88d1923ab189bcb5e596b43f242f80f774"
      "8908143829255c9fdaaa7fe768d9de340c409897746000a8ed9cf7ec6ad3ad4bf1807f29"
      "d523e4dc3112ae0e1d6bbe4b5de26cea240e4fb79e46c8106f80151defc905474c375772"
      "8a9dd1c47d4cc44a0541d1eab9726487cc107f07b66c399fd1ad8024603b8fe27a4d2fd8"
      "51780e31245bf45ed8803d36bca76195403ea70cb8a84b93e93c9a84b2bb5e2a690e71d9"
      "bfdcc3b5cfe830acc0e241df94ece4caa53dd15aa63547d08c00e2ceff07094e9a699ad3"
      "53785378d5bf46c12819caa9b9070130c12d7fe1a98de3f506d2fa186da7363b6f8ef2ed"
      "b54a229cdc120d882bad605a10336ca26237f41a03964578f426fb2b8d5031ce77bde4e1"
      "aad41cace6d44cff56136fa2e17ffbd76ac97c1f3633af228faf04b3be4bc92502deec80"
      "6fa67ce0e02f34ca122e11965920985c60f7c2b8917c026f5a83cc30d968768ebc5316cb"
      "212fd07123b4bf39823fd06b3fa101c905fed0b163b381795ed61739270edecaa53e6182"
      "6a8324c6e6663f8161a13e0210f112a29e742ee8999f285ebf9a31a65012fc5c071712b7"
      "fde50b0a5a04193858c2368e35b9736ba28cf19c9ac45a01ef0c8568d6742f49a5cad368"
      "ceb372366558d3b4ad0df2d6074dda98ed2f0e820628cc6eb1e6f0c8b2a21ff67d96f19d"
      "289a9a91d45a85aa8ead91d400710f9bfe1540da2e7fc46457eb504beb7cb082d8fcf405"
      "fcf2d80b19084ef41004b88066060157caae4fa042324b0389648b7e85cc9b2aaac70c0e"
      "6973fb23f1a1b273633ce651a6f5dc0b781e7f303e59c8929dc12251522964bcccf838a3"
      "c2e8bcc92915aaab256ccde594efe95696c76d42927594c14c4d4c86f0ec2e8aefaac793"
      "b20b07b903840fe57e997976d56b4aef760ecee113212f8525a5d0f6cd9cdb300b153f2f"
      "10c25dc1330e69415af7b429ded4c577e658c05ec39a4db0a7925991f41d74ab74f6619b"
      "19446aa870c764a85e17ef7f7674bbb0c94af82e26d50a63e72ca5404f9b5063b38f3a52"
      "311a186cdc7f1ed380d781cde9cb26109b916e9cac367503ea002b6dba796571123f6003"
      "f3f644826fbd1c56d7e29a88c72ba6cc6b3594fb59504459e57097095a3099d4de05e713"
      "5e33c931a16ebfe312152223102de579835388497213859e3ceaad02bc65fa0c762dcb17"
      "0c2fe3cfed95d695a23dd47df73c191a2ae16f2288be15e1fd529c22e4e248330954a3de"
      "268216e5f61dc4d429eabae4d85a803563d99c49a9cbe9c063ce6f659557c99fde5d0637"
      "bb3d38371b8066ca55f3c1b0906a8d75393907e957614425243107ae80dc90f2dd0f3ef4"
      "553de9aa7d92bae49bf4d0bd788c9ad866faff4f2cc9139bdf6a778e642c7b77623ec4ea"
      "7d9c9a4101ff67734388df51e5ae574c2c3b3569fc87923d5a318dc539389b8bdf0ad986"
      "cca4c0c1c724045886250dd04cead0351c13398beb089978196799514196057acab7f81d"
      "db0f676c8143015f29755e6d9b8eabdc643f648a0cc3f740df9270f37c6d459727c41645"
      "f6abb0df97cae28f0bb92e6ef70968a983b1bae67c9585d14c43437e192020e915d0ce12"
      "641e9d146a2808db7e1a833cef80620a8fc4a7af7c2bb8a7e1759c5a18db8a62dab2cc5c"
      "6285694985111ff56df2d2b5d95e62021abc909c3ec1ebe55e910a752aa4aaa246b57090"
      "d41dac7adb11eaf7d4a3cf286e632be305139310d8ab631ee3530a0d0a24741094000568"
      "52530a3a7974bff9c573dcbf8f97a916149a8e0058269e3d5164f616731a89b0c697dc73"
      "deda3dffcaf214a4bd8f8057c2ebe6b9d0f95f834af27ad6a22fb73cb5304a72bc511e0a"
      "9e618eed3b7109d80108a3dca378fae75edc0987303bd560cba495564cd98fbd0f3065af"
      "3ad04f786b799a0830d65a8d56d192af4e338cfae1711076b48cf5eaf66cfdd2b8bc54f6"
      "49c2df301d1346340db71e6d9c01d6911e2272edd546947edee3250488d55829d9603e20"
      "833334c50cc66785e50cc4f7cd94138ab71053ec02a5d2aca4513e48d1656ab3aeb74a47"
      "22d5bdee4737d487b1da2f5fd0f1905521f478665dad394aa20664118cce8bdddf0a640e"
      "de1631d5be362cb1467783beb472e49a4c7f76928623e9e01160b9ca5ae6df629c060951"
      "8d2c7218a190e8b07f4e47e3ed98058d8f96fede15c18858d14257cf200e822f39fdefd8"
      "a5ab7f826d08b84f96f0241d009da7034d7e2a592ad012ec98370533d77dc11a575db959"
      "a260bff72b1e460fefaff5826eb7646182723bc6822dcd39312699c25692e5b49216e55c"
      "75db661a7f96fe997048d54b1b0f39d495175bb740581f6a72620e202b78c65d6ddd90dc"
      "b3ba2eae32394ddedfea2f5bc20201009e81af76ce2bb9d09bd9b0524c46691e0b93593f"
      "f32636be8d97b0e7850cbb312ae15c49b0a7b35159822756740e6491fc2bf26d0c40b6de"
      "853a6c433e33f2a687d1b0c54af963bdd0fc62bb7825280a1935b1034672c742d7053382"
      "9a346aeed70eb544692bdb924d6868a7c494407a5148d28f1e2ff2cb288fd0892322d35c"
      "cfa223b47fd5ae9bf895ad79af5261c726b734d3ec813b5fb86a613c3e38580222f9dc0d"
      "8e9f3a0c1c679063b3881a3bca10e857360994d6481bb84a960cef5dda0fca386299bfd7"
      "2527c21b54a5c3a029c41c0d22b54b26ad5f3f40241d2ddb915d4819cbda291ff25eca24"
      "b5951d3e42c63afa6229acf78b1093c3883d55d554373d200d376c3cdfdd7c65baf3c919"
      "eefbecc598e322878776c2fc00088c8ff307904a10bfe29c92413ea28c6c87ef1e897b21"
      "eceb047b5856f0919b110a65075c0bc94389ef5e756c468cbcf750130cc8054efafb19e9"
      "5229cd437d0d9baeeb6ee653971a030853302077f989dfd074e9fe855994c6687aea2d11"
      "ef2b6ebf1ffc9951f9a4f86f3c94320a531f91d21d4f4354dab348bd652dbb499f1ada01"
      "abea2c028fc2cab1a0219265f815ab701f5b7ce079ca453cc2bab7422c1844c4201bdac4"
      "5a1165f08b1a3004ede39064a3986e91bc45c25a1943f04456229d59e079cbdf30ca69a3"
      "9093d1142c9305ca314d2be40e4f288b1d97bf43c527fab749acc8dd411c9f099ae91527"
      "90e955c6d6483d0a3632d2502821065c0914379057233761df6524e7646c3f9efbce57c4"
      "c2d6608dcea4d6d7ff3a64866d0144a69fbd83ae27803ce1d2b814457edf92a8567e147d"
      "c72fb8b34074b8898c30c9207fc92e4956e214378882eec06e2b339aae7007c53000c856"
      "97ab2ef36a22b1e75713bf58fc112e21fdc963d5d3e1d046591619006e7ddf5ad2f340a8"
      "cd2fb78f431044fcee8b562e7cc38cd8b88e7b045586a956cb379a6775724849871c8ed8"
      "88b361a76dce5d21652e8f2599211858473f76fc1c9dbc190b7188690636db486efc06d3"
      "656f2bf91a8b964377a4645648983e6b89967be9c7f1a6ccff3eab7b40aaf02064b3ce26"
      "824bcf4cd0cd702ee987cfb99bedda497bec1205fb2cd0a37683633c8641c4a88996ced7"
      "ee7e88c92d3b42159347d073601ac99ef23516f3996dc2073554867bf447a5a48ad9754a"
      "2e4ac2228ea3a9b256d00ddc471e8e2142967aec5a2ef5b88a4fb503d5113e3326ae8e9a"
      "66bf7897306962b7423d441891ac7c74a004aea7a5cf4fc6ed8ad749011961ea99019285"
      "d1f4331bf81e69bcbd469a1999e299f2380d0b96a74116948a75af7bd879dfa9c2878d57"
      "37299e7d465c177ac573c104e01d2b324b316d130913775bddd3a01424e0d9ac329c25dc"
      "93e2d4ac901744a4e5b42c78f0731011b85056babca5aaa0b510944d441d51a1c23761c9"
      "4d1f087260f2332a21047668022049bd47c3b159b801833d99346487838bb0eeb95cbece"
      "2e1650f9b394408c84c463f14cf7a456d90f38b75b2c5bbafce3380adf56f6edb0ae0054"
      "7178fe3da0411148353581c9f1de7b4ef51d35a981733cac5f691e6ddb60759e6572881c"
      "7deff611c3938538894be3c0c10b4b56fa422efef40ebd357888fbcfdf354b07246451c5"
      "9da48ac5364bbb68aa1e19c638b0346712e6dd60c381aef645aa58148991dc8789bc4743"
      "019688cd5f8fcf80b723833c138354914dc8d06868646ed767715cdeefa7af39333d624f"
      "2c832dbfaa302c7883f917960b5422e55b6d8937c11fd18d5834b0cb0658422033a090f2"
      "4c3dec8f75ba7912a1b3de1de85dcc990aeae2c242e65d0a88b506db89889f7a1e557069"
      "cc62c3be37ad302f35a1c80c69cfe37549255d1f0f6ad018038dc53922455b88df561eef"
      "9773fe25645b265f8843c4ff9a2a6de0d1228c798f258c3e8c36342ba4e622da0fd42be5"
      "751ed0076b74ea8031ff8ad33072028fdaae13e7198e6a1269ee017000787960296fa6e2"
      "46d855d9aec812bf8c9f3654d71f4d64f6a334ef03ab5b2d953376f3c893954e96d37ee1"
      "d77e7e5330948cf7af27b428c94a8d8cdd8a4346d1ab3c602bc0f7c5610060eb11db222e"
      "72ccc5e7562888238d4f9b07da6f4277502ee2d36caac3dfb0434362e948a6972c83d893"
      "eba110d58164b618b60d243237385524721ab417cc479e0e02d7cdcd193e051dacb0b17a"
      "f4ad85cbeda24b396dd0e250bf2f29a6adba1ddeaa78361144b6349f6382bb289ed3950b"
      "532a2e2178d6a2073310e5da054d7bb9981d6de90bb619e131aa280bb5427849abe7e31f"
      "b88b431f78a9577a70e5808e2cf89580672620b319143501b22cb5763d02f2e2f5ae5ff8"
      "fe7797672d6db97b2e77f0438797a496efb7b860ba8203a24d782131224c56efef25968c"
      "7fef294f106212266e9d0dcddf8f66fef645680a01a2c5a847fb25475348c1083a368143"
      "5190a31515b900cf79a0d117e589368532fc2d62e535757c788ab0367e22d1cbb2fa0cc5"
      "3398d5516e05f96cfca49a4f52aa09fedc599db4c4a23d0ab546b9701036a8496ee1a32d"
      "6f88dd59aaaee724ebedff3177d926deb3d96619c5aedbf9a565d44234370a38faa8d134"
      "7599074d6cedd84634b575822c8b10982b33a59ecbf3b1669640c2d4b5a8d7972c3c0ba4"
      "8b03ea30228e6788a7e6cc77250eeb001d11ca8c2fa99f698706b01d761b6c4a19b386d7"
      "eeb8f2513dd6b25447f1f3520d977f0d65932dc1cd27a90efdf4147c16b080d09770037b"
      "bbf894647ae0e081e16ccd4264c0db3d85100662d73c23e7a4d6179d3c0f07c290cc11d9"
      "00d6503384aa345cc833bdd70b391b4edc80d168891fc94de42549d85ffc5f109902f403"
      "c6efdf3d7c45c03af70713932473519409a8264c90f2c850a9bbe5be9bf7217aa52a5360"
      "e36f9ebe0f3c835c33102048ef62e963c657d4598018c0aa7fbc57031612dee15d7dec2d"
      "9eb6f71fa4ba963d14cac8eb373c5a450abfac8350c9a5f45aab61cbf50803bbbc2c");

  EXPECT_EQ(
      "0x050000800a27a726f04dec4d03f5370017f53700000100f2052a010000001976a9143c"
      "6ef7f4804a495040ad24bb5274c924ef70625788ac00000256fca85195acfc96859a47b2"
      "cff57aeb6eb71c84f60d2c5bdc37243baf8847a2c98823d9a80fa5b24c1945bef5ccfa66"
      "29ef3ef93dc367efa51a9b19269cb00045e6d46d7fdefd518b0c3b205d09439f044cba4c"
      "24aa3e8172ddb7647a0491367988a8f6d8f0b269fd173314f8256eb50e015756c1c7596c"
      "92f4daa956e7a31e8ec61d7d750c91cc8700a34c9edb975103c0bf06d7c4c29b1851739b"
      "1d865099c7a5aed5374f0ce22b76d136a80b4bda1354dc1811c7211ce6c98718420f72d3"
      "f428e869a31b445e3ab0816c79c7661f7cc8bcd9759987b61a33d28b5748900fa55d5c13"
      "925e45a03bcc7111bbb8c9f688e35f9fa888e1e044a3d34d9004b7f19ea2bc1735488047"
      "d8c60d7522e541ea0a344d9d6166030da69232c0f6ff96ec9de74b50e679d4afa624529e"
      "7d9439db4045513f34777de67dd7a4f7a73605ebf85f4240e520ef62fa21ef1e05fa187e"
      "1e594bb875e806501c2fdcc5a85c087b0cfa88271371a365151a429ba383ec37ee7ae786"
      "a66d612518851b2b6d085f0564fa2a75517d648212e34d9dc3dca2a07985627beb4a4883"
      "b637d0609de6f4b09c4cb0ec2a3405dc0b41fcd1cdadd517daa992d22c57d751162306fb"
      "a9bd1d700c55d6808e0a5bb5c0008e83910b53011afe4e4d349841b50e9c943361584628"
      "cd9e3492210e29bca3fc89efd931f2b54d5c775c87517075b34a205bb170ea572dfb1ebd"
      "6fc03baee93254b43f1f87d8bd722dc2bdf4f5763815dd476c2eb4d7c72b8f0816a6af0e"
      "35d5e1f61da3c0da19656a1ea0b1ad26221093971fe66fa7d9d3ab4a49f562213b30c3c8"
      "fb8cc94026eb7bf42c937329441f666d40db5016c36c9d367f53f7b6c56c6334829e8f2d"
      "501ede1efa0233d4ffb1ad1cb1b1bd1d0eb6270d31dae521d702ed135606f7a7abffa294"
      "ba1eb472694bdb7598b0c338e063f2065ebe08da8b0eb09a02760cb5ab93641289c0825c"
      "7bd29b2e33ff8d1b176a1ad9c785d33809877329d511773af7db831bb5d4ce44176968f1"
      "5bb37b86faf386948d1b5cfd996daca41709d4fc217dfe1a03f08a76d89a4bd36f7e7a1a"
      "6dc4bfe6432cbbc127f219d7711225919ef2a98188e7e1b268d68d76168819a7e40ef0d9"
      "fcc75aafbf57e7f70f3972d228a69c6e4d127a67733b0f11caea37185562822cffc1502b"
      "db1a044e9cb7bdd30c1bf49b8636f0a41dab6e82c23c7277de1fbcf7d69acb0cce21ffd2"
      "9a08e20d851de63de3b7d5fd9c1b0ea21f208891905e706afc79e13008acc5625adadb94"
      "cefa913974699c691b5dd78162d0b96ae32d9755f029de1ad4af996fd91cedad3b96df37"
      "553aa9aafee8053ddbf14e4473aa80b2b46d0f1f6efedd849db82f7c5b9dd1ab3ba4c0fc"
      "73bb3854ae3ed140932cbfa1e3ac479da9e5178a1a0c9388e360b6352a8d793350ba0043"
      "d70f769a8c6af945974ba0a67a905d8ba86061eb25e9a1f53313666cbad49a930493e1a3"
      "7f743fd904ad56035e58979ef2311d4349fc3784bb21e7690efff17fc240d801dac1b8b2"
      "9af646c57a3858428aff61422f61d6bdfa8d7bfd55d70302d1776b5757f33b5c1dcf4a35"
      "b0a9e43c40917ec7fbb97a971d3effc85b008ff5505c10d6c2209057840f570a25d74b1f"
      "0074b727a92b6c94b792a447285ec476160d3f47b160159fd1d327d8382af138641225fe"
      "ff56bab425aa93c734a5a7420ef63ce75c0d45af42fbb9810ce15e25b7abf519c6717e43"
      "7306631ff39a9f7d85a5c38d1d0c8cf5f9ff90d2fd14e03c6a4aa8a966a8e7245e6968a7"
      "708a6025ae2fc049516f22113f113e83f0435dd18c2948efed5816f13b81995f7ca441b4"
      "0cbb4e121faed3ebdae91c70707c8b7079e12e3721c59300495348f91692016de289b606"
      "d5506b8a52f1306780e80ce2e61a91d45dcecc302ece940b4a166623a93e33ad5dad3771"
      "65cfb5e2d1e2c6c599bfea546baf2481a1db87bdcbe084047f590b9e48ec2f3a955a29e6"
      "32a893c02f65c8016d61225c289064f868bcaf9f69bd3650e883f259b83fa1c25900d653"
      "791add8743f5a77e24bddabcaa716e2254d29dcc73b7196c1e9568f2aa873e99bb9b1e75"
      "b147220ed68918fa4e2c4032bccbc2025763d8578aebacf8b4096cd31002e6ac4e9437c0"
      "68b065709780bf05f398e7e5526414c10214216dec25e81022b976b608d413776dfd2d74"
      "496ac90162f9cd101f8c41dcacc31f0db6d063de96c71504a014ad40ca6be2bfe8e68a16"
      "ae4160f5593e360ccf2f5acc02263d67ccac8045bd235c6662a95fdb9b6b5c5b574a36f6"
      "a4ae8412ba3a6b5503982c062a010000008dd74f03661630ab41e830ee088e7d63f58777"
      "4dc9c515d8773213014023f001fd601cd187a948ad21a4838894ecafefc0ace7eb82a007"
      "a253c92c3ed67b248a2a0c00cea086bbdf94ddc4123b246c308d8a25148403c1fd9f757e"
      "a0e3804a7e746f8e89f1be9e7d17ab87bb42167592a66d9e497773a5acffe244350c06c4"
      "0c40d535fa3d6b580076d7be77c1d60f5cab41411f79215832130d73242c737c9d3f92b3"
      "8debf68f9405a517efa456c0a2fa727ff7bdd170f014fb925ed737056920e182a6eda5db"
      "3d45885e3a309859804455f671373ffa01c321cb5ec32fe4674ee60f1f1deef2d6e1d3ab"
      "51eec2287e8d602f0c9a6db75ede22d88c344bf3d67bae85738b61372f6eb3e9e9029543"
      "b252d107dbd9e10295df4d949f28121f1431c1ba349fa326d3abe266cbbeba868e7ed4ff"
      "2a41e9f005d85a094ab7bd5bdb8f06b81973defbb66a0a05bbfbddd48fb0c1702e42aecc"
      "a8d98589bf19167e13eae2af30b0f647ae3357898ff91e727d5f393b3240ce5a28c438c7"
      "cfefac30dff945174426f44bd7894d059e6475c8d079c56c944b5f6cef31f6e288133e4c"
      "946e8b21e35c61ad952e9f7bd2c60b2ab56c59fe2c352955e0bdfbb4823e985295abb184"
      "b9ba796297ae21334627240aa7df536414003c6e2a1508139354e8ea59ad3ab0046f876f"
      "03fda1c1b22f9faa1d2959477e9d9194e55b7a993d0d3d4740d8b6baeb703d3f863e965d"
      "d7582e52cae8e2792c595cd99fa4cea6f888bbd6a0d36d8c008d30f0ef39e9b142884756"
      "bd6f223c99b439cee2442bdb60d60333b9f423a9d86a59e619cc2729f1ec56512b9e13f6"
      "61a4f8de2f8a9b05a3eb2ff18abd9ca6b7376914a38b00f1eb1e099b35e0daafa782fe5b"
      "405b6cab9cafedaf99149f97b7d02da9b3d2d24d9dd616902baa3376669f953b3be03aed"
      "c52c7c38d0cb33a14042fde5d737569dffb02029ce04460045676c71faf7629587981649"
      "f80a4b11434a08165f8f54485aa6d10543a01a6a53ff86516fb47eac61e78ac3d1b29caa"
      "350b8aac7c1292c9448c9e06184dedd00b2d213c30ff87d4e147e848867e21b2ded38e85"
      "ca76d2d5d4b8a7eb5cbf277d0d6eba7c112e395be04d1484de8f93191e631d62baeaee83"
      "91794031aa0abecbd2f1a377deaa1a3487736e4d8098a6b6c88258a57154120ba2b1f49d"
      "84b4fb864e1961dfc9b016ff523334d206744c0a7cf011e7bbcef005cd08b5bb21c6ef7c"
      "511d7d9bcb8aa5c5b2a6afeb3e86570126976a32c65290b8e04f60f91b5b8fb1839d99bc"
      "08171aa731da67da3e837a072a526fb9212c7f13b3c1c2bdcef8e53f2d5d52a1505eafa3"
      "a474a32e83a83d04c20acf7c710e06601f0d445b38d35f8a464d08f41269ed44306789f5"
      "38895833b2a487d6efa6b80a7dad08a45c68a5cdb7bd38e79b00c0d3d44fca20ea04a3a9"
      "935ea11da665655e6189643aa9ebb49829aedcb33f4446ff33ec0c8bd3c0b82b4533e024"
      "386a11ff2f6392e89ef54e5728360b2f8f54d40fb3970900e3a9b53a590033bbb69434ee"
      "1a4763d57580429b158c35d3aa32924a5270937fdaf413a36e3bae78b866fa9f8628fbc8"
      "820aa5490dc54f55cb88869ce07807cc7cef74269f74841a5a57cab6623ffddec217e143"
      "3e3f3e8857f192fed479aaadb3f9aa3fdc2aa8d6c4cc7fc4f218ea598a904707b216c300"
      "7995b17dcfecffe06f8ab8b46033143cf3aaf9739d483a7049dcbb2cf20e5a3949a21daa"
      "c8412af5e62c63195fed0b77f050260652055c0647a9c620556b0ae7b20fc43372506a65"
      "27d6d5903cae4c4b6a8a9db368402c0f475f6021ce8d98f085ad632de654a41e0e074801"
      "ee957dff3cacba5ced0850369c40abb0efcd9096be772fb8cd497c5e0372db1249aa309b"
      "fc579d09b3bacb9ab95a055652ceec085a1001764f72a97721cc668ae73302d8f14d97ea"
      "1cac28b03ebde8acf12c5dd21364f5aa1cbdd884cf31018cdebf5c5a9688a7dac4af9205"
      "09c826c11096e310c56d00995705963081a8ac994b554690badb1264eb589c4a30c5f260"
      "2fdb0da081bc02c460a4a4e7d7bdb09ca109bf37ced97f39072e6ea579e138a8b7040c2a"
      "ffb6a6221d4854af26481527ec0e9e13d002f71581c3d421c9d7fcc23650a8f0abdac812"
      "b16e1dc430fe029a9ba8901fa79bb55b318023d5fd99a2953b6aa363c2039ca7434fb56a"
      "badb0d2c8e9fc2f13f5e60991cb3733a1267bd59b93b56a3ed33983742c4de57cbc26a1f"
      "725b2c61bae179332cd58dda30f1f5b3ed73ec2cef898fe8c9417360b6f5f5252561cef0"
      "3c2fe9c74c5cbf6cd30b72a718c021317bd0240810bc7e1c754dffab00a2d767922b25ee"
      "15747a0d845de4d0c88f46b86397ac46d8af79228115eb8b89f4da69e32bf3ea53d7bc5a"
      "6d34ae43ad86ade87b8cba8bc6c94ae96bb4c336d4fd94e5be1aab6ead30c7ae513477ba"
      "118462845abbe0cd878dfe6d1d73bf0adf020e7f423ee37c4fe0035d4047ab6b2a4ec07c"
      "b3cbc620bac9051e466d32130cd4372a64cff887bc541f555799bb6cbe6ac2c5d217ff5c"
      "a0ac06dd452e8e009c109921077db5421b52ea0631ef1519b15a97b86af22487bb7c46b2"
      "960a60017ca743bfc2184e291d49195d8305a1d5d0c1fd959385a74046ce4dca6ca6152e"
      "165baf161e533be476be6d2466a1fb213b9f6fddb2ded5a74dffee95735c651944bc93f6"
      "0a86c21b330514bc5038924126d54ddcd9a8fefdd1fc047f4c7c673cb517b13f56d8332c"
      "c741dff5e893146fe70c5fbc4825e8bd0ecbbe84766827134e552841709f2517c195619f"
      "b3c3cef82cb321cf55459cff75e92d01acbe5631519f18348df919e3161b99947ecb2023"
      "05b8332359437bcb63e229cc8b5e9c0c90d7762226a7a36fd58024e07985cb824558ec6b"
      "7520690d9e2f65a341b48809c3e489c17b8286185d1ea542bdb863c8facfa9a8f5448077"
      "b468e690c604243a252df3da7e7e1479c19262d0ae9f6cc36133e3f680ce1ee13c9c7a4f"
      "312cf813cf430bc00e30b252bb73e3a6275392d0dd721d37ed71cb485fbbd9618b524527"
      "843709aa6bd4c25480fde0bcf9919f44e4e12098317f20634a026046040840265b11c36e"
      "316233ca7df8688b7fc957918e524503766551c243295daea36718314fd65d29dc1bb0db"
      "2b51a7c01a632ea12281a8ad9970982abbf3e2a6b1119e1742beb37fb001203c75c54bc3"
      "802d78e770a87397ecf57c10a4089789f4bccb1189c257a51467344cbaa4692da1374ebc"
      "49ac8293c2a23c6e40cf6d76966f4c2efefd929a74bcbfe71bb5d8b0a517c2b0823b1606"
      "259cff5c21477ec71ea1b00b1f2bba2756569b550af53ecfc399824feb769a90b96ba3e1"
      "c4e6afdb5b8b64392d150ae5adf7f8a1793b8dd87420fe0cf95fb8e64a06f1b2ede3161d"
      "7e80253909ff05d1daa9e5cfca07a8f06f64880e3b1747f7755489d0f2f80607566c4d0b"
      "065b0a2317718307865f6e548b63ae09c316f2d833dbe5ea943f820848c44926e6710f8c"
      "6a0c023af9f45670728db1aaa1966084fe780f7b86819286fac07f1f4c181a6f8e7d4e88"
      "7efaf78efe460c7e64f34e6ee9c6f0ece91cb57169db5b093d7e37c511cdc811d48c3c6e"
      "5304f269157216b3db870302eea61eb31e2b300829f62c25e49ee5805dfb240d07a4223f"
      "0578c9d7905d143b893e629ee0f34f15e1b9c5f779e1692d3acaf0482f72c41d4cdcfc5b"
      "c7f76a8429482aebd4c3f02c0757fdf4752dc9f7b472c2cb550c45afbb4a639cfc0b21d7"
      "c7a74bb22752fd04848c86d149c36fce4fb5ec159257a467b1aeefaf87a7328e4032a194"
      "a2390d1117d1185383b0dc7ba3e9e899c16e893a40e5051be88a84bdaeb1aa8e3337b82e"
      "36acfffe989a1556049c1ee14ec710d3d5eb0fe4af3c573498e7aad3de7f8d25639f3ddc"
      "3daeef2dc48686be40aa575711041351808ffa5220dd3b2f6a49f936a1b4a3b7e3897dc2"
      "96e65b4295bfb00d1bafda26038d540c9a06cc7682b0c11c428dad7493ad80fe30319b02"
      "1171f8e80982ecf80edae8a35269627dc021582d005e1dad64848419ea2a47903e76d9f4"
      "80caeb5d7810c09a397be69a15dad53b54a41bf97209242762d34330a4fe01cd11476b0c"
      "f950979cbb0fd4efc0c6da0461bfba874996b2733b8dd7eea8384792403db4d7f742e19f"
      "fca2f6cc07d5f83da004985a9af389855cd05a295cac6de29e434936c3c99bc3d58c5e8d"
      "7793b4195113bea73ebe48af62876accbf5d76e6ec1517c6b5325542421abd9ffae67f00"
      "2bc87af7a8eec9bab731694a641f74ff3397dda42d85984406857775ed018c12dbd86391"
      "00f04766b3b39d24eb399de67e66d18c83c88e346aa2594ccd8eb91f6c5333c35e529013"
      "e994884d30dcd229b3d4fe8fcb3a38d3fb2327def48b102d7571cb07cfe798fec9e28972"
      "ec3ce4c1eb6f84f94b6a13b2649b5ece6abb2f29d683c8bdb38adfe9a4a4520413bd1914"
      "76286094d47b2627ff98287225ac09117ed1e4852ea1da0ab0eb7216f7b1d608c27be33c"
      "0225b44e6b3cc37edafe9f2e7ee050c1fbcba913e4adfa4c2c6250ff9699642703405f9d"
      "3cae85d995fa58182d4333962e0ad8d177e46d25e94b1701236e4bd8b0b62bb4548d8bec"
      "f6c9de1d572d893937fca52ad2efc5c11743acd4539102a73aae74544006d87d4cdd2227"
      "ac24486fc6585ada3d04a917fc84f8fe77937828c2029cf4653eb996621eb514e18d81b6"
      "6a85136ad5f6ba0dddd170cee50881c0c9f6260f09786a601518bb3c46afe03a0ffbbe4c"
      "ba777c0dac10471cdf1e9ea8c5d9f97765ec724f90a03e30fd04dc0edc3d4cad76107bd3"
      "0043971cee3c149be8556dbc0e081ca1fc318c3f561658ea320f6ef68cc57e1d867853a1"
      "c992669d19baa2ccc26e0a18320f25294a59a73e40147d56e9fc136aef6df6fd3328732b"
      "34548ca5c582bd596265db294830e9054f0e217123f8920c4b9e066945d97707fe4845cb"
      "f2380bae7b64663a17bf9773a5abf49f2d9884b2f2d3d71529a19a0cdf13ba1ff37ce5a8"
      "6801ba3a30f474d7c6a122b1bcf7ee74e6e413facf89e20bb469c6a0f2bdd67d34c6883a"
      "d1f9969d3378ca3087e0bc7aac32095dd1c6fb8ec6f768a07ebfdc72aedac3080dbd0459"
      "c18ac8abfd88d1923ab189bcb5e596b43f242f80f7748908143829255c9fdaaa7fe768d9"
      "de340c409897746000a8ed9cf7ec6ad3ad4bf1807f29d523e4dc3112ae0e1d6bbe4b5de2"
      "6cea240e4fb79e46c8106f80151defc905474c3757728a9dd1c47d4cc44a0541d1eab972"
      "6487cc107f07b66c399fd1ad8024603b8fe27a4d2fd851780e31245bf45ed8803d36bca7"
      "6195403ea70cb8a84b93e93c9a84b2bb5e2a690e71d9bfdcc3b5cfe830acc0e241df94ec"
      "e4caa53dd15aa63547d08c00e2ceff07094e9a699ad353785378d5bf46c12819caa9b907"
      "0130c12d7fe1a98de3f506d2fa186da7363b6f8ef2edb54a229cdc120d882bad605a1033"
      "6ca26237f41a03964578f426fb2b8d5031ce77bde4e1aad41cace6d44cff56136fa2e17f"
      "fbd76ac97c1f3633af228faf04b3be4bc92502deec806fa67ce0e02f34ca122e11965920"
      "985c60f7c2b8917c026f5a83cc30d968768ebc5316cb212fd07123b4bf39823fd06b3fa1"
      "01c905fed0b163b381795ed61739270edecaa53e61826a8324c6e6663f8161a13e0210f1"
      "12a29e742ee8999f285ebf9a31a65012fc5c071712b7fde50b0a5a04193858c2368e35b9"
      "736ba28cf19c9ac45a01ef0c8568d6742f49a5cad368ceb372366558d3b4ad0df2d6074d"
      "da98ed2f0e820628cc6eb1e6f0c8b2a21ff67d96f19d289a9a91d45a85aa8ead91d40071"
      "0f9bfe1540da2e7fc46457eb504beb7cb082d8fcf405fcf2d80b19084ef41004b8806606"
      "0157caae4fa042324b0389648b7e85cc9b2aaac70c0e6973fb23f1a1b273633ce651a6f5"
      "dc0b781e7f303e59c8929dc12251522964bcccf838a3c2e8bcc92915aaab256ccde594ef"
      "e95696c76d42927594c14c4d4c86f0ec2e8aefaac793b20b07b903840fe57e997976d56b"
      "4aef760ecee113212f8525a5d0f6cd9cdb300b153f2f10c25dc1330e69415af7b429ded4"
      "c577e658c05ec39a4db0a7925991f41d74ab74f6619b19446aa870c764a85e17ef7f7674"
      "bbb0c94af82e26d50a63e72ca5404f9b5063b38f3a52311a186cdc7f1ed380d781cde9cb"
      "26109b916e9cac367503ea002b6dba796571123f6003f3f644826fbd1c56d7e29a88c72b"
      "a6cc6b3594fb59504459e57097095a3099d4de05e7135e33c931a16ebfe312152223102d"
      "e579835388497213859e3ceaad02bc65fa0c762dcb170c2fe3cfed95d695a23dd47df73c"
      "191a2ae16f2288be15e1fd529c22e4e248330954a3de268216e5f61dc4d429eabae4d85a"
      "803563d99c49a9cbe9c063ce6f659557c99fde5d0637bb3d38371b8066ca55f3c1b0906a"
      "8d75393907e957614425243107ae80dc90f2dd0f3ef4553de9aa7d92bae49bf4d0bd788c"
      "9ad866faff4f2cc9139bdf6a778e642c7b77623ec4ea7d9c9a4101ff67734388df51e5ae"
      "574c2c3b3569fc87923d5a318dc539389b8bdf0ad986cca4c0c1c724045886250dd04cea"
      "d0351c13398beb089978196799514196057acab7f81ddb0f676c8143015f29755e6d9b8e"
      "abdc643f648a0cc3f740df9270f37c6d459727c41645f6abb0df97cae28f0bb92e6ef709"
      "68a983b1bae67c9585d14c43437e192020e915d0ce12641e9d146a2808db7e1a833cef80"
      "620a8fc4a7af7c2bb8a7e1759c5a18db8a62dab2cc5c6285694985111ff56df2d2b5d95e"
      "62021abc909c3ec1ebe55e910a752aa4aaa246b57090d41dac7adb11eaf7d4a3cf286e63"
      "2be305139310d8ab631ee3530a0d0a2474109400056852530a3a7974bff9c573dcbf8f97"
      "a916149a8e0058269e3d5164f616731a89b0c697dc73deda3dffcaf214a4bd8f8057c2eb"
      "e6b9d0f95f834af27ad6a22fb73cb5304a72bc511e0a9e618eed3b7109d80108a3dca378"
      "fae75edc0987303bd560cba495564cd98fbd0f3065af3ad04f786b799a0830d65a8d56d1"
      "92af4e338cfae1711076b48cf5eaf66cfdd2b8bc54f649c2df301d1346340db71e6d9c01"
      "d6911e2272edd546947edee3250488d55829d9603e20833334c50cc66785e50cc4f7cd94"
      "138ab71053ec02a5d2aca4513e48d1656ab3aeb74a4722d5bdee4737d487b1da2f5fd0f1"
      "905521f478665dad394aa20664118cce8bdddf0a640ede1631d5be362cb1467783beb472"
      "e49a4c7f76928623e9e01160b9ca5ae6df629c0609518d2c7218a190e8b07f4e47e3ed98"
      "058d8f96fede15c18858d14257cf200e822f39fdefd8a5ab7f826d08b84f96f0241d009d"
      "a7034d7e2a592ad012ec98370533d77dc11a575db959a260bff72b1e460fefaff5826eb7"
      "646182723bc6822dcd39312699c25692e5b49216e55c75db661a7f96fe997048d54b1b0f"
      "39d495175bb740581f6a72620e202b78c65d6ddd90dcb3ba2eae32394ddedfea2f5bc202"
      "01009e81af76ce2bb9d09bd9b0524c46691e0b93593ff32636be8d97b0e7850cbb312ae1"
      "5c49b0a7b35159822756740e6491fc2bf26d0c40b6de853a6c433e33f2a687d1b0c54af9"
      "63bdd0fc62bb7825280a1935b1034672c742d70533829a346aeed70eb544692bdb924d68"
      "68a7c494407a5148d28f1e2ff2cb288fd0892322d35ccfa223b47fd5ae9bf895ad79af52"
      "61c726b734d3ec813b5fb86a613c3e38580222f9dc0d8e9f3a0c1c679063b3881a3bca10"
      "e857360994d6481bb84a960cef5dda0fca386299bfd72527c21b54a5c3a029c41c0d22b5"
      "4b26ad5f3f40241d2ddb915d4819cbda291ff25eca24b5951d3e42c63afa6229acf78b10"
      "93c3883d55d554373d200d376c3cdfdd7c65baf3c919eefbecc598e322878776c2fc0008"
      "8c8ff307904a10bfe29c92413ea28c6c87ef1e897b21eceb047b5856f0919b110a65075c"
      "0bc94389ef5e756c468cbcf750130cc8054efafb19e95229cd437d0d9baeeb6ee653971a"
      "030853302077f989dfd074e9fe855994c6687aea2d11ef2b6ebf1ffc9951f9a4f86f3c94"
      "320a531f91d21d4f4354dab348bd652dbb499f1ada01abea2c028fc2cab1a0219265f815"
      "ab701f5b7ce079ca453cc2bab7422c1844c4201bdac45a1165f08b1a3004ede39064a398"
      "6e91bc45c25a1943f04456229d59e079cbdf30ca69a39093d1142c9305ca314d2be40e4f"
      "288b1d97bf43c527fab749acc8dd411c9f099ae9152790e955c6d6483d0a3632d2502821"
      "065c0914379057233761df6524e7646c3f9efbce57c4c2d6608dcea4d6d7ff3a64866d01"
      "44a69fbd83ae27803ce1d2b814457edf92a8567e147dc72fb8b34074b8898c30c9207fc9"
      "2e4956e214378882eec06e2b339aae7007c53000c85697ab2ef36a22b1e75713bf58fc11"
      "2e21fdc963d5d3e1d046591619006e7ddf5ad2f340a8cd2fb78f431044fcee8b562e7cc3"
      "8cd8b88e7b045586a956cb379a6775724849871c8ed888b361a76dce5d21652e8f259921"
      "1858473f76fc1c9dbc190b7188690636db486efc06d3656f2bf91a8b964377a464564898"
      "3e6b89967be9c7f1a6ccff3eab7b40aaf02064b3ce26824bcf4cd0cd702ee987cfb99bed"
      "da497bec1205fb2cd0a37683633c8641c4a88996ced7ee7e88c92d3b42159347d073601a"
      "c99ef23516f3996dc2073554867bf447a5a48ad9754a2e4ac2228ea3a9b256d00ddc471e"
      "8e2142967aec5a2ef5b88a4fb503d5113e3326ae8e9a66bf7897306962b7423d441891ac"
      "7c74a004aea7a5cf4fc6ed8ad749011961ea99019285d1f4331bf81e69bcbd469a1999e2"
      "99f2380d0b96a74116948a75af7bd879dfa9c2878d5737299e7d465c177ac573c104e01d"
      "2b324b316d130913775bddd3a01424e0d9ac329c25dc93e2d4ac901744a4e5b42c78f073"
      "1011b85056babca5aaa0b510944d441d51a1c23761c94d1f087260f2332a210476680220"
      "49bd47c3b159b801833d99346487838bb0eeb95cbece2e1650f9b394408c84c463f14cf7"
      "a456d90f38b75b2c5bbafce3380adf56f6edb0ae00547178fe3da0411148353581c9f1de"
      "7b4ef51d35a981733cac5f691e6ddb60759e6572881c7deff611c3938538894be3c0c10b"
      "4b56fa422efef40ebd357888fbcfdf354b07246451c59da48ac5364bbb68aa1e19c638b0"
      "346712e6dd60c381aef645aa58148991dc8789bc4743019688cd5f8fcf80b723833c1383"
      "54914dc8d06868646ed767715cdeefa7af39333d624f2c832dbfaa302c7883f917960b54"
      "22e55b6d8937c11fd18d5834b0cb0658422033a090f24c3dec8f75ba7912a1b3de1de85d"
      "cc990aeae2c242e65d0a88b506db89889f7a1e557069cc62c3be37ad302f35a1c80c69cf"
      "e37549255d1f0f6ad018038dc53922455b88df561eef9773fe25645b265f8843c4ff9a2a"
      "6de0d1228c798f258c3e8c36342ba4e622da0fd42be5751ed0076b74ea8031ff8ad33072"
      "028fdaae13e7198e6a1269ee017000787960296fa6e246d855d9aec812bf8c9f3654d71f"
      "4d64f6a334ef03ab5b2d953376f3c893954e96d37ee1d77e7e5330948cf7af27b428c94a"
      "8d8cdd8a4346d1ab3c602bc0f7c5610060eb11db222e72ccc5e7562888238d4f9b07da6f"
      "4277502ee2d36caac3dfb0434362e948a6972c83d893eba110d58164b618b60d24323738"
      "5524721ab417cc479e0e02d7cdcd193e051dacb0b17af4ad85cbeda24b396dd0e250bf2f"
      "29a6adba1ddeaa78361144b6349f6382bb289ed3950b532a2e2178d6a2073310e5da054d"
      "7bb9981d6de90bb619e131aa280bb5427849abe7e31fb88b431f78a9577a70e5808e2cf8"
      "9580672620b319143501b22cb5763d02f2e2f5ae5ff8fe7797672d6db97b2e77f0438797"
      "a496efb7b860ba8203a24d782131224c56efef25968c7fef294f106212266e9d0dcddf8f"
      "66fef645680a01a2c5a847fb25475348c1083a3681435190a31515b900cf79a0d117e589"
      "368532fc2d62e535757c788ab0367e22d1cbb2fa0cc53398d5516e05f96cfca49a4f52aa"
      "09fedc599db4c4a23d0ab546b9701036a8496ee1a32d6f88dd59aaaee724ebedff3177d9"
      "26deb3d96619c5aedbf9a565d44234370a38faa8d1347599074d6cedd84634b575822c8b"
      "10982b33a59ecbf3b1669640c2d4b5a8d7972c3c0ba48b03ea30228e6788a7e6cc77250e"
      "eb001d11ca8c2fa99f698706b01d761b6c4a19b386d7eeb8f2513dd6b25447f1f3520d97"
      "7f0d65932dc1cd27a90efdf4147c16b080d09770037bbbf894647ae0e081e16ccd4264c0"
      "db3d85100662d73c23e7a4d6179d3c0f07c290cc11d900d6503384aa345cc833bdd70b39"
      "1b4edc80d168891fc94de42549d85ffc5f109902f403c6efdf3d7c45c03af70713932473"
      "519409a8264c90f2c850a9bbe5be9bf7217aa52a5360e36f9ebe0f3c835c33102048ef62"
      "e963c657d4598018c0aa7fbc57031612dee15d7dec2d9eb6f71fa4ba963d14cac8eb373c"
      "5a450abfac8350c9a5f45aab61cbf50803bbbc2c",
      ToHex(ZCashSerializer::SerializeRawTransaction(tx)));

  EXPECT_EQ(
      ToHex(ZCashSerializer::CalculateTxIdDigest(tx)),
      "0x496dfffff625ada462cbc8a733f305fdef1ca584ceb8e7efa5e28e38249b466e");
}

TEST(ZCashSerializerTest, OrchardBundle) {
  ZCashKeyring keyring(
      std::vector<uint8_t>({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}),
      mojom::KeyringId::kZCashMainnet);

  auto key_id = mojom::ZCashKeyId::New(0, 0, 0);
  auto address = keyring.GetTransparentAddress(*key_id)->address_string;
  ZCashTransaction tx;
  tx.set_consensus_brach_id(0xc2d6d0b4);
  tx.set_expiry_height(1687144);
  tx.set_locktime(0);

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_address = address;
    tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                   244, 58,  55,  220, 160, 120, 126, 62,
                                   201, 246, 96,  82,  35,  213, 186, 122,
                                   224, 171, 144, 37,  183, 59,  192, 63};
    tx_input.utxo_outpoint.index = 1;
    tx_input.utxo_value = 115000;
    tx_input.script_pub_key =
        ZCashAddressToScriptPubkey(address, false).value();

    tx.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::OrchardOutput output;
    output.value = 100000;
    output.addr = {212, 113, 78,  231, 97,  209, 174, 130, 59,  105, 114,
                   21,  46,  32,  149, 127, 239, 163, 246, 227, 18,  158,
                   164, 223, 176, 169, 233, 135, 3,   166, 61,  171, 146,
                   149, 137, 214, 220, 81,  201, 112, 249, 53,  179};
    tx.orchard_part().outputs.push_back(std::move(output));
  }

  std::vector<OrchardOutput> outputs;
  for (const auto& output : tx.orchard_part().outputs) {
    outputs.push_back(OrchardOutput{output.value, output.addr});
  }

  OrchardBundleManager::OverrideRandomSeedForTesting(0);
  auto orchard_bundle_manager = OrchardBundleManager::Create(
      std::vector<uint8_t>() /* Use empty orchard tree */,
      OrchardSpendsBundle(), std::move(outputs));

  tx.orchard_part().digest = orchard_bundle_manager->GetOrchardDigest();

  EXPECT_EQ(
      "0x5af5dcc1436a1746e8a702a1d7763e8c7b2857f0037c2bcf3b02bea8c36fe6d5",
      ToHex(tx.orchard_part().digest.value()));

  auto transparent_signature_digest = ZCashSerializer::CalculateSignatureDigest(
      tx, tx.transparent_part().inputs[0]);
  EXPECT_EQ(
      "0xd55f293e03d6f5b9e7275becb99749d5268b650bf193da862123bc5b0fd50dd9",
      ToHex(transparent_signature_digest));
  auto transparent_signature =
      keyring.SignMessage(*key_id, transparent_signature_digest);
  ZCashSerializer::SerializeSignature(tx, tx.transparent_part().inputs[0],
                                      keyring.GetPubkey(*key_id).value(),
                                      transparent_signature.value());
  EXPECT_EQ(
      "0x47304402203b9fdd43a88444b0d649079e21d0c8e71176ae23298696dd164bb5551f05"
      "182c02206f8321ed7a738e864f7b79e94310d50714be99e82148f1e4dc5feb39903e5780"
      "0121022b19592aabd4f5cff59b4f842ae09155cb0440019ad1fa80086ab35b3acf565b",
      ToHex(tx.transparent_part().inputs[0].script_sig));

  auto shielded_sighash =
      ZCashSerializer::CalculateSignatureDigest(tx, std::nullopt);

  EXPECT_EQ(
      "0x58fec6023369e7fe37e800ab83fda11e182dd7100a6a204d164a950bd3f72dca",
      ToHex(shielded_sighash));

  auto orchard_raw_part = tx.orchard_part().raw_tx =
      orchard_bundle_manager->ApplySignature(shielded_sighash)->GetRawTxBytes();

  EXPECT_EQ(
      ToHex(tx.orchard_part().raw_tx.value()),
      "0x024330112c31d0cbb0db4f77cefc9036ed215ea86ba2aee9a5de08953a8216282f5136"
      "e62a35a7a471c0a68f7a2b09915993244010af0ce69e863a0cdc68042b103dd62d315080"
      "c0bec76397d7c6ad365d84a6354d07049e484d0240410c589389eeb33ccbb564cc383b01"
      "383d4bac03648d6f155fc5d3d280f0f85a3687f17b0e90a1dc6793503417e53794980e4b"
      "8b5e1a18e454da912fa17818988a7b0b1006fedd657b795cc584843cb90f9cdf67912f20"
      "17dccdb3858439dbea6fadbeeabf0ac892bc6f2c59cc9344d9f1b781671ae7c9df2f4f69"
      "fce6c8215fa7feae4be87a0b5bb08266bead9d4ee0f80433cd17b713643a5f02ce53392c"
      "b015418fc4ff336ea32d7a749448802b7f828d51d51d78e707d268959c856d9239a6eff5"
      "090d1813581a19c24df421a03eea8bd7e9f8a9cf938c35fe65cf213a1a4b6c727e9401cc"
      "519f3ef3f2c936079149bc2af3083d8a963811c761138e44383c08c34de0d483f62879c4"
      "cb80c5cb84124da36abe113d68eadd017c5c268d6fbdd9c661753fe5375c8043fa3e9f9c"
      "94a8e1035bdd2e8eaa3a814d0a62657bc5fe689568b5a41ab718342b4790520e1bceb54b"
      "987d2590354b54fe695ca24c6585bb591cdc2ac395f06a07194812aeceaac0cdbc4178b6"
      "8be11e30352956b250378495bfaa4b326d264db71b83b28ef164300dee9a17f187617596"
      "c071f63d9383aeecec6f28e1080013e674b2412d15dd667e2532339caed06f4db6da25e7"
      "fcc326a1a58469d94df082166dbb24a1117898bf7c497466690d13949cd1db05a516622c"
      "b2d746469d04e716acebb04f1c519f00d26dac539a71c2e4c9f1efa0cec80ba0cf37a29c"
      "d375e0fa70146becfc66ef4cace008fc8a22ae77aa718ce5a149256df6407a24d33e5eb1"
      "e9d51b2bd2ba4d71b71f3bb54538e984f719bbb2daebec2a54476a4104daa47d591fd9db"
      "9820f68d48cf52dee3f8772fdd25de4ae436ea1cf125296c30e91262b407c80188224c35"
      "6572acdcca3f41fcefb5e618993b1214a542a06425439ac7c8812be6a0bb8ccd9abedd75"
      "202451ca522cff1643fda3e399bf60b2b7393a6f89dd8dca6a1c1ede5965975a62c0088d"
      "3a5e68c0b635535c053509c7f55bf9261ecf9b5c2ca045762df3966fe4c1e97af1c6e3a1"
      "ec3eb3f9b57bfdf44069a9d45105bc00a04f80f9e15f02b34aa42ca95ec8527ea52d52ce"
      "05750a1d67216a3b2f334682f65ce5ea49599bd7e01f98d14c07095635740c3bf644a7d5"
      "eb61c16d28a5bdfdfacb311a29f6b62fe50b71864f70937ba483b96ab0c4f6c566ddd1bb"
      "55536a33a20a8b4f9642f67e8e16662017b947df57320a51cd1141d8e46a40a8c9de24b3"
      "0c0b08957050b693dc18c5acc1482e07204cf113ac974dde5d62044ad29910f5a59256ad"
      "08352c7141f6b65043a86e93a3a25fdba8f6f72b60a9e5276da3e20f4e36573ed72aedcb"
      "4ae68ac15a576671933c1a102d172031fb0d586b1ffb70828ff5a3ff02b38d67153388bf"
      "cf5b6d02bb476225fc50786083d09fad59b0b97da176b7b4787277d4f90e090b8e6f370b"
      "38eada32da32734553d64b4681c21af7ffe2bcb8d914e43cf24a953a0f94edd975409bf5"
      "927de38e324ea2e52bf4590b0391dff3ded4354d0bfe2ae3acb4a9302124c73718ca2d2a"
      "30667be950fad78bb9fe49f8ffa40afb40a2ab26ecd17bc9ed16dc0e848550825fd093c6"
      "f1c4eafcb1c7c9859602f7fede5fbe5becf84f28bf74abc880e56b77ed7a7242448da180"
      "f07ffe854a7fa321a2722cdb9c1c46b93186ad15001b32c0a7de4a02a1526b334d97c2b3"
      "6f6d0fe83d3f219837c2adc186d6ef1a3f1e0a50d6ca2fc1c2c2a2ea95373d6a2fd9906f"
      "49296f3e8bc141a8c6bbc904722c2541756622d7284e45697c6f0f1a141b786e80fe8823"
      "91d7c5919bb98e2fa1719da21c9e4d04a0c30888b0b608d46a054c32b75bee0396efdeb3"
      "10e8c5211d41728ab796a44cdb543468e0d655cf64afd62e71dd4688eb7e2e196025f1ef"
      "0c2cbc99cc08075cb36b99cca1600edd3f6451eedff9c3adeb7b843deb83a5582e51ad32"
      "881b8ca6064986a41d26d91d0c48adef928d766c7f58bd9a31da0096c7bf18a9f8c307f1"
      "4bc82ae3601b82407069e4dd8a2c4b72909e2810435816ed3747e85302c90381e0dee223"
      "2d6c21b55310b76fbdaba0aa2deb8a1c25641604b1bf523281c8f7c5a80a93d967ced997"
      "8830b45c2dbf64d9ca8cdc6498082ae003d50eafee3c930d40d253f4bfe65ec69e7260d3"
      "b99dbed28acab956ba79ff0107d586c878760caa9ac9036079feffffffffffae2935f1df"
      "d8a24aed7c70df7de3a668eb7a49b1319880dde2bbd9031ae5d82ffd601c6d8bb1d60347"
      "0a10040bb50333498933da7cac03eb16937cef9d35d9371046b2d13387711416e9c51d06"
      "6f32fdfae7203369a1449da2700c37bfdb099d24a2be7f8abd0833762284617c9150b81e"
      "04d4051e0aaccc7100179b997b8fa7bfdd305d4183f78f8385336c189426bf0ff42bb5c7"
      "66a93d2ebb4f3fe89835da45c106c9dc861332d6f3b6705239303cd9b60210a9a2665329"
      "34511322f6ba8eac228a2df4ddddd778bad54694afc2f7d4159a3363361c4b677d723a7f"
      "b56a6f0f2914e7710b0ba8b8d1c209db43bb4911677f1bc1f7fa3faf5d4498753761c08b"
      "5129fe69223b94ebb00548ab19310d28c02a3930c68b0cdfa548106476c7d98b46b56e44"
      "1701fa115d58c76a523ee2711d13b2b75e45ac466ed3ca501b690d89a92047c72cfe3dbb"
      "ddd9ba784458ac8520e369993b31535f3585b7e0679a5dabbcac553e904db5b8f5884679"
      "ff1fd49ff4cb9f57ee3fba42e5dfce4de71f26f11c94da728f9d159ce803c145fd1fa307"
      "0d6c6d4318d303d050334374a84eaf66ba2f9672239e32ad301a542d3d569c94d23a9a9b"
      "424069c4a66261fee5ff3c66282f6ed9e89898f5ca0c0b26ff37f21633bf35cacfc848fc"
      "192a7be3641eeadd1d34641912dd53e4400391bc5341ff004f14dbd35fe96960f86062df"
      "c49092178ab0b05a87867c4656825f5c818da35d74628ec85d08d398644573789a1018c1"
      "a21a4acd1012a10086b00f96d6083abfd79337c0a3c44dae81cd14c2a4cad615668e0407"
      "bd305f77179d26b17bf314c0437848334eaab3ea92f27ed59ef928b7d03b5bf1b910033e"
      "b416772a28e363999839175d75bb67ad350df97ad2d20a262e2e8c72e0ee08371d838264"
      "5a4237e2327d6c1ed602a04e0f5658776884234c32b15a98a7cb4310cacbdfddf80cbde4"
      "28705cd193a7bd2fcae9b8541f06557348226a5d18f9668b4bd86ef9171305ad9649ad36"
      "481766eb4da18047bb78528fc839226c0f4b13815fd65501a5f43ba7f5cac5249115baa6"
      "c78d6922a92035497f93377b38483e9902ecb495027105254667ac1039e6abb8a70ad170"
      "642d79004bb9e5dea33d6caa8faaf6ce0f8edd82b5db64401228e8e590f2af64bfb58dc7"
      "fd07f449e6a57fd1245c5f3b21cde2e32d73f28497654d53fe4a9fc6a9d9c5e07f1f8b1c"
      "ec16aa279ae4a89b88a58f04854b5c29fe6ddfce9906f8fbdb3454fdcba69b599935523b"
      "8dbffa48e0ebbb6c7e49f8b6cde4991ae5ae8e73ad4e3c88d1be12dae46e9c23a50ce7ae"
      "0b9e07c1a9efc075dd2d7e2a215042b19ddc8c28f41a20415620825bccfb9cfa07e1280b"
      "646d591a863fe6b7b007a8b946e51062a3370aed325d2e7dda1ae4bf5c412c41b2aada6f"
      "583af999aa55e63ab52672218c1efa4197e73d1e1eaf8cb9a0f0f790db781952ecc670a0"
      "bf0cac8285ec02133c206d186cafd560b6de563c813d57993db2e57733999c09e1171dc6"
      "f9c3fa3a4da170b4e8895587f389d24b395b5eeca1d8bb9445e022624cfbad4cf20b115c"
      "549e8e6235bc3ebeb07c30275dedb5fa2912f948b372c4a58c51afe63f62eb90e73d7f51"
      "e8db044719959b769753725c1e67975c8ccd51a5480c8ef45dd5aff6bda0038ac67132a3"
      "74fa6d6b75e0a4941a9e362fe6f5f2dc72f8066ca9489c81dc2fecc7c3c14dc19c8f89c7"
      "cf38dcb0f89c95819c022f7765de3eb69c28c29d2b2a428018cc80a0c8ef8698ed8d2568"
      "19f8dcac8b2b5cc499fc10dec93deef6fca8c2d80ae283f9668db5329eb2cd1c9cc92ec7"
      "79cf0014c8bffc5b4f283ba484369f057e0c075d5276dc6731186889941fbe199076c458"
      "0dca0bdafaacd7c6c13494d42c948abeede8dad58dce340e041296befb48b3a2738f0948"
      "cfc1e556043283154a9779c3fff7afb164b44af1e13738ee3544cb8c34530e9408278eea"
      "c1249f7491983b21b09e85536cc62132105f98030f4b64827db9e9e4e2184d80100ef264"
      "e5e22708fd7c7e8cb3333fd64f8296076c3c870c6e5e7d836b6c9ae1b8a1bc8f3a712ea0"
      "431273a24c245a449e9c8e441343029afec9edeb101c1d8c9dbffabc1d17c6818d4d4f7b"
      "96217bc5c7aeb843ae250aa8f946dade5df1e9e6882695ef58e43995b8f68b6fec10c01b"
      "0fc4a37d94e9c3880effda57ccdf9d72e5a1206288db08812639cf3a3b3bfa76646af70b"
      "7927e49cff1977a4368021c0f604b836e9b876f6b9140f5045e88f268bf798a72b04b8df"
      "18a4aae9be79c27d483eb39f7632a18a351d87870fa1c5105af75dff136f44e5edfa6e77"
      "e280330c51a8e841a8a3581b60794cfbe6ca3998373ea4a2a589d6346a9b0988ba57a852"
      "368d762ef9c4c3de464f971212a4ec6fd7dfa9a525d0be17f43b97d3240b09b33c3e8cc5"
      "ae2dbc2a736af44d55d3acb18c226d292157d6a59243483efb6547464208004e07dbd0d0"
      "4f4487d1fdeda1a3df63a31239beadf1bb0368a4677d5147ef2d18a9a91cf0105dccbf56"
      "f04516bca3fee8194c9b76ae71ae2f2f2e2cfc58c70dbf6054c721b0c5b08551b0387c3e"
      "82ca272128c70ccd0057ed4eba9f9d08663753d57d8c5ce309799361f8009ede59963f46"
      "57b2ff7c95b997e577f6242ebe1c137c1c3f03b4eba314097927e7fb98fc9ba76c141bbf"
      "fa705526a511e75ca52d08c1bd88908df10d307adf603119db56217c2db92d47643d972c"
      "6e5c431d5d32c0783e464bc058a2e3207fec16f0a0cfe44d5586260b82a625b383710a37"
      "fa33f662733dd364e49137712cd57223b76773cfcfdf845b7659ccdada5211c2f12889a4"
      "465caf3f138d7a90b2d202c0451aef799175be5f3af781272e730985c62c30e445005801"
      "3e0b2ca758c0b27e9213788cf187a1e4ec8ad456bdf9a0308305f98f304560216d3a95f0"
      "78291e66cb9e8f6276cb758b4940a997b6063d41372cbc9bc67edc56510d572eb0bf7d35"
      "226dc5795fc6c952cd6799212ac527907e18fe78da43121faa1e3ef77d39bb201a5fd779"
      "6c8bc04b0dfd6a84d9d12cda260b183bd0fde7095c1a78ca0f3de835f91c1b075356bb04"
      "4f61f90ecaae69b9293fd8560912245cf7914230706f611dfa4c47a19f2b2472922fddc8"
      "667b9cdc4f212c4e1e63d8308ef9558e248141e4006be8b860a049435ad183427619c0a6"
      "fa15e81b8a16a438f061a178381791e4026127c378a1299ab51937bea30f99ccd63be778"
      "05e274e597571f7ed0dc534eb700605ea3b2a5f6d31e4f228c6fcd56fc2abfb234dccf80"
      "351ec3ad9024d844bbc3b22b476fc846465c900e9ac021594e0c7e46cd1ad81a6b6aa4dd"
      "5921adad1f15b87b2b0b0236ab0356dd7e127c854d11fcebdf43cce59acd70dc405a98ad"
      "51dd6cc2858baaa81eb5a56abab22799c30c152627af052ef94c1d80f28afe0aa1498168"
      "07baf5bd1c85da9d40d2336ee0287461a026266af7cfaa64b952a5eeea67117db0243c33"
      "747a68e60e07facbaa01d6e4e7f992493b594b2b0d839705e8942207b2983a9ac8084734"
      "42a9fb47b3292cae218986071254c4065eb41c28fd4e68c0c55b36984ebe755d712e5014"
      "672b88b3efd71c9a70b1b2af8da4c5484f0708aa9eb7d64f42dc08bf5cbddb2d1e38831d"
      "7a511ec2f4026caf2e443a86f2f9afdaa21b681d80088e98500f96f0bd31ad386015b85d"
      "ceb49180ed9360b862b3dacc3613c83b5bdf636c55c7a2e5a22a9a94ac085197b741b650"
      "f262a6ca4a02db4ee1bf04387b1cfe838a2013f66a026b101842fa4e44965a80bcc9ff05"
      "69503a39fe35e7469a90541674a04440e70353ac24a7d2d98862da84c34fb7b79983e209"
      "f29e4ad6684deab378079333f42c1513a986063d8243d453148c42f10f88fdf4610fada4"
      "91b781b7a1d9d890ea1883862bd620399b1611d543613d2202fe86fff14c155568461498"
      "d0780e3ebf1fe2c0f2f857f6cfa2ac4ca1088cc7051463dd4fc6c1c297b627c54af3ea97"
      "250af20e585fa06fe2212f751f397073e8ab67630a75bb5c4d6f2f343b4cf0ba851c16fb"
      "904e5a1fe66f0b2e8bac618fd2cc490c83216efe87f759705feef7e1df3c0ca649f641c1"
      "488aa2fdfc4159fc34c0d61b06c9d5ad1816bf7210361497d73392ab294d3e6214deb3e6"
      "429037ef38add1d172b9157ddc7d078f5578704da7292a03cffff71145c5124a8fe9a46a"
      "7da10c071196d1f9175c6e53585756980124ed7744922acf08378ad0776d4e82df020b14"
      "d1c2384ece2091373a5a0c8b7301391a4c73066178a1515285b2e4f1482858ea91c59af4"
      "9bc5e3d268f5140ae02d78edfc715fb5052b8fa99d2fb2287832231ed38d98432f5afdd9"
      "57a06ad11b0a58da42228f3106508cb04702ce79ffe526a5f5dee1e9d026ced1ee06eb92"
      "1109120034556432b89f6565d10f4b9cf7248faeddbd94a317da93ab8d7c93524d0fa2d2"
      "70398acd02d36a43f6e8e460b4fbec62194b73469be9066968cbc10bea2acb1b98369a38"
      "42975b940fbc875476c1fa7400ee95733ac6eeddf8353e70d60c164d4b229bc361d7e38a"
      "60453445190caf1dd3684c491efa3e0abee650c21c37734b6f8cb2ef851e7e8d98aa5d23"
      "99f1768a2cc81020b59371d16510bc1e38374922ef26159f629fd7bd7bc40b3ddb4ea2a1"
      "c7bc4da6504f83beca6e5163a12f72b26ed6fd219e5edb7d316f712311f57ee16ebdd215"
      "3e981a4890ab63440208b3dbbcf7f886ea217cb82608a1f606559268d4f0d1ab16f4bc0d"
      "cef971d8d3218bfccab87f704de90c418ebbda7f088f4a52130d0fd12bcbbbe6465b96ad"
      "8e111042560de47aad79bd1473c882bad2fce71b4c78a3b99e4f9b405e84fddd6815c6c4"
      "059cfb4b896880fb01e01fee33c9113a075c1107824b342545f984d90d0cd29e951dd482"
      "ec068a6c4fbc02f86ba5ca1bda0d847972287e6715cf5975d135c14bc8a9703b1aa6294c"
      "44a50979d0d789fa997a41f00899002a0f2928b6bf2b57dda37ea5d38d06e12734b88f56"
      "6e5ac6d9be1f48afa815d7f2f0841d3177105b348379525fcc9a6a5fd14953ca3947ca43"
      "cde56eabf0be2867679a632c291b8354fa0e4a0f9ba5ba2f2122c4e31d2001c0ea858b46"
      "6dcc3c1c45cf8b19392073a45ed261f64878021d18f43520238d0205dee2cd3e6ef878d4"
      "07f5f5cb3832204f4b80ee92174bf3f470c6e1800499d8d16f03c0d2946c460127a873dc"
      "46021ceeb84c34d136efdfdce8283daf3aeab90aa4f1256d5509838c96349cb88f1b817c"
      "94177e89271f3c464b7dab45868a80f52740ec504334b87b1ffe7315d13e76ea8379873b"
      "89c652e7ac0984475231e71c67144a0c59924f96dc800d09b00a3a27f86f62f79317f7fc"
      "439a5e235fce80908a60256543b5f3564a8205500b26e4a7d35bf60251ad8d52c8d44a6e"
      "3d0b111aaccbd813f1aa042b554b911bd3385e26be1af29cd0d3afa204e922a2ee039875"
      "df3fdb5c65636939a186304f9e3095bb82cba060b043e138f36ff275e00ca27ec7a57a91"
      "853f19ee520d9a3a9e1ab97d57683e6091229180b6167d566982ef0865a70e487e27116d"
      "7ca227e7500765d4ca712cffd73dea70f138714221f308548c2fe9697a5a8204cb00dab4"
      "55223b21dbda4ace383b4c2d5cb8a80b43a1cf80a85bcdbae443e7cc65931b7d9c395a09"
      "d85aa04a0b896a1ababc3de81b258f22af403b0e45923c8ef7d2f68aa224aeb1261cfa4f"
      "645649c5f9597037eebfc066e517495d983e1f0461c651b4e425aca81ac92f7d624688b6"
      "15d61b0dc20bacf56710803c549ebf9a558f652ba4052026c5725e0f17a016eee0e070c1"
      "a857b8bd6d920397072b5cda654653a4ae18167007127ede81aea309b7a226293309449c"
      "3df4bb06bcfeab20b9fd0f7a682a557269bfb1030c7d87fa7337fec56a7126e553a65869"
      "b94b8d41925d9fe4ca15de3b916d497206bf80fbd657658c3cd59efe2d3e16f9886d48eb"
      "21ac931e9831fef5e1d8b0c28b0935378da0ab01455fb9e7e0305b638995f6dc7c29cde4"
      "293f871fd0318419a5fd289a1dbc78bff788fe84c5d85b0eabde25192665f1a9611dafc3"
      "64dd2c76548e56c0c09f33ddcddacdb858e689ef60bc9024069c836cd9231a25c204a450"
      "c7a141ca2b6283b46f39aab490f368d5ecd8dbc2856ef94d48106550406fce98f9cac770"
      "3694869d9e9d9d56adadd6e5d7938503fed3e7f11f13ec55c87f9582e6b6b803f4359e4f"
      "1b1f0fd64137140da024169208957ec32301644b68fea7730fe73b03be378ab66d6ba0d9"
      "a95f44572e577ce0af4d5b169d0f5262e70a8056d0e9c9aa8c7e199b0853126de575fcba"
      "6c7157789f46cd81870369218875e27ed132b80688035798300e01d621ce19a80a712a15"
      "2d04eb4a1329eef9c2a76b3d8ef87aa5a4e5a0df0b5d86b5f6cf2a30a14e0eea06fc7bbd"
      "9f31bb761e527b1a302a709e048db0829bd834cfc38ff0d8656eaf35dde2948b9001848b"
      "61ca80842be9eb2d821f4968e4a5ba17c407d0614e305b4867eb10f6fe0fde52c620f171"
      "ee0e8431d95c2fe6ebea9dd3edef4e38e1c06a447f238aaeb00aeb3a2deeaf11def8c703"
      "06e30f361000bbab84bd2b8d00f7489e11cef6ccf22b2f73f5df1bd8ceaa96c72c2f7347"
      "9ef034457bd48f180f8c25885f7317578934adb4d88d0b9e0716f011c7ca6bdba21d9a22"
      "87bba225bdac9ff40456ecfe0c110dd2a1d02ed119a5219b5532def6e59da498dd52fd04"
      "50fc239de044a71e773c66435a1abb5a717ddb44bf0bfe38e606f3eebf545a6932428ac4"
      "e03891d6d124f97eb62b06e3ba4dd1c881a96de13e30a1dd4f609568cf3509c4aeb66df6"
      "ba0aa368ba6840f117e499d1a2db7f300623ed3fadde6513cd414a8767a153f8e514260e"
      "5fddc12f74cd7b2abe5a8a12268d3e0f0ce4aeb1cb02619795b7a9564f04bf7a66751f45"
      "b8c55581fdbb1def6bec2bc591361e6c5f655c154bebe62bda03d0cc13781351a6383481"
      "7768044125204b2ccbe4a34ad8787913d19c51c5fe119692ad88701e41f410d2e551b4b4"
      "485cc6104ff8f3a96e60d441b981914b6f3a6e0a103874f5c8b5279c822380e1388f8c4e"
      "051806df43d088919b6a96651a1fd18978d9f7b7303d81086fc802e80dbacddca3c67f75"
      "c31f9296a6a15671b828ffa2144d7d28aff2ef13981c08f29ceeeae61a4abf670a177bb6"
      "6ee9eab7ba051fcea86c57279c74792c0db9d7be07b568a29be798e5cabe3f5922cb9bfe"
      "291d5cc733bedd48be1e0b42580ce82b2d192bc91cf834eed96e2640493f452eb90eebfe"
      "decbb6eef637fdfb36c58fbbf19f34d15c965e67f1bd19f8a86954190813ec1f32f9858a"
      "d282f3c3bb9e6599a60ff5a450bbee8a0fda246f1d98ec49f71d3dddd8a3b89412327982"
      "0f5e5eab5ee35be19ccdba7484205dac689e5af43f30de1eea0415c1d5844265ceb11975"
      "5aab460fbb69450d1385e0917389dd2a8a254c9e11779fd80912f282c97cda530d180258"
      "8e37c57ebbc0fdd6b34afcd00008e3f2860c029e87bd0c9cf9b7c875c78138e181c6b821"
      "80f1c2a324157dc1821a868d1fe05337df91051b0a712662f8174fd1363548e629e1a7ac"
      "2468ef99a4249640812f0cc2793c330e7ae5d8e6f58ba598a75d378dd30fc43825e54938"
      "2f0998cc63a91bd59562fc019463e2ee0288ce8b972d02bfd6931c71e96f1438941f73a9"
      "fb0645e21bd830afa898cfadeae7092dc212ba3af1bfede863ac1868072674b962c28f9f"
      "fcc7c0b41eb3ce3f9b233bfeb7c73c1af34eb770f211e36d213d1327c5e115ec13741598"
      "df6924ecd5c49d92198e0690e5853163868077885525e2149476914ecd2da886cca521d9"
      "d4326fd0767d4760e02ebd300cd94f158439b3bce3e80daada688d0d82201e03704a0b7e"
      "817b822ff42671ab1178c0fbef25872f62408a9918696ebfaa016ac83ab0ae6d7e564e4d"
      "ab9cdf72c2be75e11a2e3bd421fab646c18bffe2b414db0ae47a2178b1fedb660994277d"
      "f443daba0335987650196c1459fec34300902fa7e093a319815ae9ca77a05e36cfc6ca1e"
      "921d38e5ce1d435f0780add12a7866d0e09039ee77696fa71b98b2ddf7cda608aa3d711b"
      "36c0d7b7f00562173c3b1fff6d4b8bda89b59946a0702a1ec432d3369e03707faa4c8e40"
      "725e3a93b1b3250990aba031b44511c2427f69e82b75f5ebdf39365ef9785786c9db0219"
      "b79f2520478d52490d3cd7f8c369f82ebd7401588c282ee868d6a578cc55fe5bbe83f32e"
      "a45b1da7169669275ab8de98ff73a74296359fcfba3a8b0fc1ba5c077096896e30077237"
      "231fb2573b4b564fb2f4eaab2b1c23da8f350ae14bbed8724bf9f0a99634fbfb703448d3"
      "0d0e23df250781c655397703676e87a1c194988c4c986c72fda703fb3a3e6147e35a50e8"
      "2780b7299218452340cf7493fc24fedf145ccdc6db06dd4d5bfd42cb9651b9530109d467"
      "8f110beb595c0d34e894bcd2fed27f0d5bd494852591ad791987f09f1484a4d504052943"
      "5f83c2ca21a450f450eb7641c8555a7ad97932d2bf415190e91ca48a9d2ad7c5f66198b4"
      "0c33f2319aa0f877081061399259a141abe5b8501e3e4597960d65f6e1d3779cbc2f2962"
      "d86cfc3a791b8f7b819fa9eac682de213ed1fe66ed39f7f5be6973d00ee63c9d6d7ebcd4"
      "a418b2ba25eb7790f8a9be0849c254e7123d05f8a95a277ab53929d5b270349b51fdd4b0"
      "0cdb7f90843da55e01544592f637c94a736de566ce1febb0f7ac8996b0b5f8d2681ead3b"
      "9a20705c79793445fc22f993a1e80a5cef83bd897dc24974e9dd779d6f8a532f614a6886"
      "e424eef1be0a2b93fffd2a0224f8efbc65306d275419a42395fed06402da279a57cf7127"
      "b6171548af2ec8c14000eca0ac27c16779a6f75600121b7461f4c9df3c10e33b649f5b4c"
      "b450c9d18fdd199b1817f092f4709e33e3bcdcbb5ab10345568bcb4a7537187e85111007"
      "4a356ac21e4bc7456b5c9039a0a8cc82655a5ad404d0a8aeca1f7c6432972230d605468c"
      "674b86e1ef0e84677733db6d44ade25808d3802ab1114b56e553bc8ae55f75fc61ce203a"
      "d84726396eb136932b66d0c948ee67929c1abfedefe6b7d6799743e0d6971f518315652d"
      "5ef0b780da21fbf4b99d543d1d122ceac03d3e08584d692aedba5250387f71159efcf212"
      "0779c907bb358badcf9f72bf7817cf8f5936c19a5159cdecbabd6a7954dc46ed2dd50b58"
      "264f823b8dbfcb37e5a5fc8d418d9e144fee2a4bd1dec0ad923ab884f229e3aa247011f2"
      "c78f46c15ceacb0cd5b579d425ab9478411df734ed88962b8ba32abd9a4e7e02299736dc"
      "2c4c6303c80bb434a062461eb7b9fa7dced4e993427744488fa96d03cf019230ddafcc0d"
      "32a14c2808eff7dfb87a5088c0dd3a838394b696b33d8b5d9823923a58ad90a24582ea95"
      "485ea91296e22c646448a4baf64ab5b2f831aee378ad5e05444029f3b885160e2790ab96"
      "566f008dc84582921951e0ee071b51a5c1ab2b81d1e30e2b2511ab57d85413895fb33b62"
      "71f742b5d2a14c4ebbbdbcb1f8a7ba0e6efef4d95c1ac0d6e400a96e468e6ab948e73d8d"
      "ae59e30ff39742598d3c64cada3a37d590784368d3db26cdbaa3185f8085a8264b10ccbc"
      "43ec9f6eeb363f006f169f0aa4168acde5599e16b161b885761f0c9dcfd2b33fdbbe86f2"
      "a88bef56fc46dfb075e01c84304ad1ea44aca68032de60acc3875e4b41630674ca07a941"
      "d72ec4c212e885edd4a7ae77910059d344fd0545cdfb4170b2b735d575a56d743fc7e46d"
      "ad742b4dfdd228cd30b2f49e86761f6c57f6d292a040444817b74d341127d5ca13ed730f"
      "c558f182b994a37565f0e05765df6004e7f288a54f311003c965b06dd41de117f54d41a0"
      "482d3276fe3ed5535b228e95c12135b7d190b11c4f56fb77f1136ec0d76acf538abdd7d8"
      "fccfe0d69d23c46942283ec4c996b87e421d8219b3fa486d6d0fd83577b8e58f222d3528"
      "65df77f25515eed1c8bdf7c7bf97eee9ac8e4b7ab35342cab4a96ad1be12e936a048e20c"
      "9c86a3281a32663f1552ce084cecf9cb1bcbfc8cff5a90042be4bf8eeeef16db1ba48f2c"
      "04998b9a4e896405f50d5e1ba8a4c490bcdf07187b90940bc36f973671d349aa772ffe6e"
      "708866f97ea7af8fb582e6031c12c999a48fa092af5b5c18690a2577c58ac5447b20ab04"
      "13c6620b3928ece273a1d107e78b5e2f3a7489e1b871685814048f5dd9f03771ccca4fe9"
      "f8d717687a13c6b6a1444417985815f3a850fb0f6d36a1ae176b49a4a154fdc436821c5d"
      "6b1eca1012c46d5c6458fca872a2ad3e12befaab636abd61bf440b5d3decbfead9ac3ea4"
      "a8f6342fa4dbb663401836c67a126fe16fb35eee5104ee9d4b2da52d52e42d62ad866286"
      "a32305904e1f267f3b0e71696b8eb1892a2bf0bbf0bc86abf6144d12c3f90a110d69766b"
      "14dcc06f8c20b9ef4bfa181390c2a78cc3b72c3687caaa7f3e4135dccb63cd2467d49af7"
      "6784f6ace55bd2bc12bca438a4d99807e91d5d1571742922b099a46ebdea8f161720");

  EXPECT_EQ(
      "0x050000800a27a726b4d0d6c20000000068be190001146b9d49dd8c7835f43a37dc"
      "a0787e3ec9f6605223d5ba7ae0ab9025b73bc03f010000006a47304402203b9fdd43"
      "a88444b0d649079e21d0c8e71176ae23298696dd164bb5551f05182c02206f8321ed"
      "7a738e864f7b79e94310d50714be99e82148f1e4dc5feb39903e57800121022b1959"
      "2aabd4f5cff59b4f842ae09155cb0440019ad1fa80086ab35b3acf565bffffffff00"
      "0000024330112c31d0cbb0db4f77cefc9036ed215ea86ba2aee9a5de08953a821628"
      "2f5136e62a35a7a471c0a68f7a2b09915993244010af0ce69e863a0cdc68042b103d"
      "d62d315080c0bec76397d7c6ad365d84a6354d07049e484d0240410c589389eeb33c"
      "cbb564cc383b01383d4bac03648d6f155fc5d3d280f0f85a3687f17b0e90a1dc6793"
      "503417e53794980e4b8b5e1a18e454da912fa17818988a7b0b1006fedd657b795cc5"
      "84843cb90f9cdf67912f2017dccdb3858439dbea6fadbeeabf0ac892bc6f2c59cc93"
      "44d9f1b781671ae7c9df2f4f69fce6c8215fa7feae4be87a0b5bb08266bead9d4ee0"
      "f80433cd17b713643a5f02ce53392cb015418fc4ff336ea32d7a749448802b7f828d"
      "51d51d78e707d268959c856d9239a6eff5090d1813581a19c24df421a03eea8bd7e9"
      "f8a9cf938c35fe65cf213a1a4b6c727e9401cc519f3ef3f2c936079149bc2af3083d"
      "8a963811c761138e44383c08c34de0d483f62879c4cb80c5cb84124da36abe113d68"
      "eadd017c5c268d6fbdd9c661753fe5375c8043fa3e9f9c94a8e1035bdd2e8eaa3a81"
      "4d0a62657bc5fe689568b5a41ab718342b4790520e1bceb54b987d2590354b54fe69"
      "5ca24c6585bb591cdc2ac395f06a07194812aeceaac0cdbc4178b68be11e30352956"
      "b250378495bfaa4b326d264db71b83b28ef164300dee9a17f187617596c071f63d93"
      "83aeecec6f28e1080013e674b2412d15dd667e2532339caed06f4db6da25e7fcc326"
      "a1a58469d94df082166dbb24a1117898bf7c497466690d13949cd1db05a516622cb2"
      "d746469d04e716acebb04f1c519f00d26dac539a71c2e4c9f1efa0cec80ba0cf37a2"
      "9cd375e0fa70146becfc66ef4cace008fc8a22ae77aa718ce5a149256df6407a24d3"
      "3e5eb1e9d51b2bd2ba4d71b71f3bb54538e984f719bbb2daebec2a54476a4104daa4"
      "7d591fd9db9820f68d48cf52dee3f8772fdd25de4ae436ea1cf125296c30e91262b4"
      "07c80188224c356572acdcca3f41fcefb5e618993b1214a542a06425439ac7c8812b"
      "e6a0bb8ccd9abedd75202451ca522cff1643fda3e399bf60b2b7393a6f89dd8dca6a"
      "1c1ede5965975a62c0088d3a5e68c0b635535c053509c7f55bf9261ecf9b5c2ca045"
      "762df3966fe4c1e97af1c6e3a1ec3eb3f9b57bfdf44069a9d45105bc00a04f80f9e1"
      "5f02b34aa42ca95ec8527ea52d52ce05750a1d67216a3b2f334682f65ce5ea49599b"
      "d7e01f98d14c07095635740c3bf644a7d5eb61c16d28a5bdfdfacb311a29f6b62fe5"
      "0b71864f70937ba483b96ab0c4f6c566ddd1bb55536a33a20a8b4f9642f67e8e1666"
      "2017b947df57320a51cd1141d8e46a40a8c9de24b30c0b08957050b693dc18c5acc1"
      "482e07204cf113ac974dde5d62044ad29910f5a59256ad08352c7141f6b65043a86e"
      "93a3a25fdba8f6f72b60a9e5276da3e20f4e36573ed72aedcb4ae68ac15a57667193"
      "3c1a102d172031fb0d586b1ffb70828ff5a3ff02b38d67153388bfcf5b6d02bb4762"
      "25fc50786083d09fad59b0b97da176b7b4787277d4f90e090b8e6f370b38eada32da"
      "32734553d64b4681c21af7ffe2bcb8d914e43cf24a953a0f94edd975409bf5927de3"
      "8e324ea2e52bf4590b0391dff3ded4354d0bfe2ae3acb4a9302124c73718ca2d2a30"
      "667be950fad78bb9fe49f8ffa40afb40a2ab26ecd17bc9ed16dc0e848550825fd093"
      "c6f1c4eafcb1c7c9859602f7fede5fbe5becf84f28bf74abc880e56b77ed7a724244"
      "8da180f07ffe854a7fa321a2722cdb9c1c46b93186ad15001b32c0a7de4a02a1526b"
      "334d97c2b36f6d0fe83d3f219837c2adc186d6ef1a3f1e0a50d6ca2fc1c2c2a2ea95"
      "373d6a2fd9906f49296f3e8bc141a8c6bbc904722c2541756622d7284e45697c6f0f"
      "1a141b786e80fe882391d7c5919bb98e2fa1719da21c9e4d04a0c30888b0b608d46a"
      "054c32b75bee0396efdeb310e8c5211d41728ab796a44cdb543468e0d655cf64afd6"
      "2e71dd4688eb7e2e196025f1ef0c2cbc99cc08075cb36b99cca1600edd3f6451eedf"
      "f9c3adeb7b843deb83a5582e51ad32881b8ca6064986a41d26d91d0c48adef928d76"
      "6c7f58bd9a31da0096c7bf18a9f8c307f14bc82ae3601b82407069e4dd8a2c4b7290"
      "9e2810435816ed3747e85302c90381e0dee2232d6c21b55310b76fbdaba0aa2deb8a"
      "1c25641604b1bf523281c8f7c5a80a93d967ced9978830b45c2dbf64d9ca8cdc6498"
      "082ae003d50eafee3c930d40d253f4bfe65ec69e7260d3b99dbed28acab956ba79ff"
      "0107d586c878760caa9ac9036079feffffffffffae2935f1dfd8a24aed7c70df7de3"
      "a668eb7a49b1319880dde2bbd9031ae5d82ffd601c6d8bb1d603470a10040bb50333"
      "498933da7cac03eb16937cef9d35d9371046b2d13387711416e9c51d066f32fdfae7"
      "203369a1449da2700c37bfdb099d24a2be7f8abd0833762284617c9150b81e04d405"
      "1e0aaccc7100179b997b8fa7bfdd305d4183f78f8385336c189426bf0ff42bb5c766"
      "a93d2ebb4f3fe89835da45c106c9dc861332d6f3b6705239303cd9b60210a9a26653"
      "2934511322f6ba8eac228a2df4ddddd778bad54694afc2f7d4159a3363361c4b677d"
      "723a7fb56a6f0f2914e7710b0ba8b8d1c209db43bb4911677f1bc1f7fa3faf5d4498"
      "753761c08b5129fe69223b94ebb00548ab19310d28c02a3930c68b0cdfa548106476"
      "c7d98b46b56e441701fa115d58c76a523ee2711d13b2b75e45ac466ed3ca501b690d"
      "89a92047c72cfe3dbbddd9ba784458ac8520e369993b31535f3585b7e0679a5dabbc"
      "ac553e904db5b8f5884679ff1fd49ff4cb9f57ee3fba42e5dfce4de71f26f11c94da"
      "728f9d159ce803c145fd1fa3070d6c6d4318d303d050334374a84eaf66ba2f967223"
      "9e32ad301a542d3d569c94d23a9a9b424069c4a66261fee5ff3c66282f6ed9e89898"
      "f5ca0c0b26ff37f21633bf35cacfc848fc192a7be3641eeadd1d34641912dd53e440"
      "0391bc5341ff004f14dbd35fe96960f86062dfc49092178ab0b05a87867c4656825f"
      "5c818da35d74628ec85d08d398644573789a1018c1a21a4acd1012a10086b00f96d6"
      "083abfd79337c0a3c44dae81cd14c2a4cad615668e0407bd305f77179d26b17bf314"
      "c0437848334eaab3ea92f27ed59ef928b7d03b5bf1b910033eb416772a28e3639998"
      "39175d75bb67ad350df97ad2d20a262e2e8c72e0ee08371d8382645a4237e2327d6c"
      "1ed602a04e0f5658776884234c32b15a98a7cb4310cacbdfddf80cbde428705cd193"
      "a7bd2fcae9b8541f06557348226a5d18f9668b4bd86ef9171305ad9649ad36481766"
      "eb4da18047bb78528fc839226c0f4b13815fd65501a5f43ba7f5cac5249115baa6c7"
      "8d6922a92035497f93377b38483e9902ecb495027105254667ac1039e6abb8a70ad1"
      "70642d79004bb9e5dea33d6caa8faaf6ce0f8edd82b5db64401228e8e590f2af64bf"
      "b58dc7fd07f449e6a57fd1245c5f3b21cde2e32d73f28497654d53fe4a9fc6a9d9c5"
      "e07f1f8b1cec16aa279ae4a89b88a58f04854b5c29fe6ddfce9906f8fbdb3454fdcb"
      "a69b599935523b8dbffa48e0ebbb6c7e49f8b6cde4991ae5ae8e73ad4e3c88d1be12"
      "dae46e9c23a50ce7ae0b9e07c1a9efc075dd2d7e2a215042b19ddc8c28f41a204156"
      "20825bccfb9cfa07e1280b646d591a863fe6b7b007a8b946e51062a3370aed325d2e"
      "7dda1ae4bf5c412c41b2aada6f583af999aa55e63ab52672218c1efa4197e73d1e1e"
      "af8cb9a0f0f790db781952ecc670a0bf0cac8285ec02133c206d186cafd560b6de56"
      "3c813d57993db2e57733999c09e1171dc6f9c3fa3a4da170b4e8895587f389d24b39"
      "5b5eeca1d8bb9445e022624cfbad4cf20b115c549e8e6235bc3ebeb07c30275dedb5"
      "fa2912f948b372c4a58c51afe63f62eb90e73d7f51e8db044719959b769753725c1e"
      "67975c8ccd51a5480c8ef45dd5aff6bda0038ac67132a374fa6d6b75e0a4941a9e36"
      "2fe6f5f2dc72f8066ca9489c81dc2fecc7c3c14dc19c8f89c7cf38dcb0f89c95819c"
      "022f7765de3eb69c28c29d2b2a428018cc80a0c8ef8698ed8d256819f8dcac8b2b5c"
      "c499fc10dec93deef6fca8c2d80ae283f9668db5329eb2cd1c9cc92ec779cf0014c8"
      "bffc5b4f283ba484369f057e0c075d5276dc6731186889941fbe199076c4580dca0b"
      "dafaacd7c6c13494d42c948abeede8dad58dce340e041296befb48b3a2738f0948cf"
      "c1e556043283154a9779c3fff7afb164b44af1e13738ee3544cb8c34530e9408278e"
      "eac1249f7491983b21b09e85536cc62132105f98030f4b64827db9e9e4e2184d8010"
      "0ef264e5e22708fd7c7e8cb3333fd64f8296076c3c870c6e5e7d836b6c9ae1b8a1bc"
      "8f3a712ea0431273a24c245a449e9c8e441343029afec9edeb101c1d8c9dbffabc1d"
      "17c6818d4d4f7b96217bc5c7aeb843ae250aa8f946dade5df1e9e6882695ef58e439"
      "95b8f68b6fec10c01b0fc4a37d94e9c3880effda57ccdf9d72e5a1206288db088126"
      "39cf3a3b3bfa76646af70b7927e49cff1977a4368021c0f604b836e9b876f6b9140f"
      "5045e88f268bf798a72b04b8df18a4aae9be79c27d483eb39f7632a18a351d87870f"
      "a1c5105af75dff136f44e5edfa6e77e280330c51a8e841a8a3581b60794cfbe6ca39"
      "98373ea4a2a589d6346a9b0988ba57a852368d762ef9c4c3de464f971212a4ec6fd7"
      "dfa9a525d0be17f43b97d3240b09b33c3e8cc5ae2dbc2a736af44d55d3acb18c226d"
      "292157d6a59243483efb6547464208004e07dbd0d04f4487d1fdeda1a3df63a31239"
      "beadf1bb0368a4677d5147ef2d18a9a91cf0105dccbf56f04516bca3fee8194c9b76"
      "ae71ae2f2f2e2cfc58c70dbf6054c721b0c5b08551b0387c3e82ca272128c70ccd00"
      "57ed4eba9f9d08663753d57d8c5ce309799361f8009ede59963f4657b2ff7c95b997"
      "e577f6242ebe1c137c1c3f03b4eba314097927e7fb98fc9ba76c141bbffa705526a5"
      "11e75ca52d08c1bd88908df10d307adf603119db56217c2db92d47643d972c6e5c43"
      "1d5d32c0783e464bc058a2e3207fec16f0a0cfe44d5586260b82a625b383710a37fa"
      "33f662733dd364e49137712cd57223b76773cfcfdf845b7659ccdada5211c2f12889"
      "a4465caf3f138d7a90b2d202c0451aef799175be5f3af781272e730985c62c30e445"
      "0058013e0b2ca758c0b27e9213788cf187a1e4ec8ad456bdf9a0308305f98f304560"
      "216d3a95f078291e66cb9e8f6276cb758b4940a997b6063d41372cbc9bc67edc5651"
      "0d572eb0bf7d35226dc5795fc6c952cd6799212ac527907e18fe78da43121faa1e3e"
      "f77d39bb201a5fd7796c8bc04b0dfd6a84d9d12cda260b183bd0fde7095c1a78ca0f"
      "3de835f91c1b075356bb044f61f90ecaae69b9293fd8560912245cf7914230706f61"
      "1dfa4c47a19f2b2472922fddc8667b9cdc4f212c4e1e63d8308ef9558e248141e400"
      "6be8b860a049435ad183427619c0a6fa15e81b8a16a438f061a178381791e4026127"
      "c378a1299ab51937bea30f99ccd63be77805e274e597571f7ed0dc534eb700605ea3"
      "b2a5f6d31e4f228c6fcd56fc2abfb234dccf80351ec3ad9024d844bbc3b22b476fc8"
      "46465c900e9ac021594e0c7e46cd1ad81a6b6aa4dd5921adad1f15b87b2b0b0236ab"
      "0356dd7e127c854d11fcebdf43cce59acd70dc405a98ad51dd6cc2858baaa81eb5a5"
      "6abab22799c30c152627af052ef94c1d80f28afe0aa149816807baf5bd1c85da9d40"
      "d2336ee0287461a026266af7cfaa64b952a5eeea67117db0243c33747a68e60e07fa"
      "cbaa01d6e4e7f992493b594b2b0d839705e8942207b2983a9ac808473442a9fb47b3"
      "292cae218986071254c4065eb41c28fd4e68c0c55b36984ebe755d712e5014672b88"
      "b3efd71c9a70b1b2af8da4c5484f0708aa9eb7d64f42dc08bf5cbddb2d1e38831d7a"
      "511ec2f4026caf2e443a86f2f9afdaa21b681d80088e98500f96f0bd31ad386015b8"
      "5dceb49180ed9360b862b3dacc3613c83b5bdf636c55c7a2e5a22a9a94ac085197b7"
      "41b650f262a6ca4a02db4ee1bf04387b1cfe838a2013f66a026b101842fa4e44965a"
      "80bcc9ff0569503a39fe35e7469a90541674a04440e70353ac24a7d2d98862da84c3"
      "4fb7b79983e209f29e4ad6684deab378079333f42c1513a986063d8243d453148c42"
      "f10f88fdf4610fada491b781b7a1d9d890ea1883862bd620399b1611d543613d2202"
      "fe86fff14c155568461498d0780e3ebf1fe2c0f2f857f6cfa2ac4ca1088cc7051463"
      "dd4fc6c1c297b627c54af3ea97250af20e585fa06fe2212f751f397073e8ab67630a"
      "75bb5c4d6f2f343b4cf0ba851c16fb904e5a1fe66f0b2e8bac618fd2cc490c83216e"
      "fe87f759705feef7e1df3c0ca649f641c1488aa2fdfc4159fc34c0d61b06c9d5ad18"
      "16bf7210361497d73392ab294d3e6214deb3e6429037ef38add1d172b9157ddc7d07"
      "8f5578704da7292a03cffff71145c5124a8fe9a46a7da10c071196d1f9175c6e5358"
      "5756980124ed7744922acf08378ad0776d4e82df020b14d1c2384ece2091373a5a0c"
      "8b7301391a4c73066178a1515285b2e4f1482858ea91c59af49bc5e3d268f5140ae0"
      "2d78edfc715fb5052b8fa99d2fb2287832231ed38d98432f5afdd957a06ad11b0a58"
      "da42228f3106508cb04702ce79ffe526a5f5dee1e9d026ced1ee06eb921109120034"
      "556432b89f6565d10f4b9cf7248faeddbd94a317da93ab8d7c93524d0fa2d270398a"
      "cd02d36a43f6e8e460b4fbec62194b73469be9066968cbc10bea2acb1b98369a3842"
      "975b940fbc875476c1fa7400ee95733ac6eeddf8353e70d60c164d4b229bc361d7e3"
      "8a60453445190caf1dd3684c491efa3e0abee650c21c37734b6f8cb2ef851e7e8d98"
      "aa5d2399f1768a2cc81020b59371d16510bc1e38374922ef26159f629fd7bd7bc40b"
      "3ddb4ea2a1c7bc4da6504f83beca6e5163a12f72b26ed6fd219e5edb7d316f712311"
      "f57ee16ebdd2153e981a4890ab63440208b3dbbcf7f886ea217cb82608a1f6065592"
      "68d4f0d1ab16f4bc0dcef971d8d3218bfccab87f704de90c418ebbda7f088f4a5213"
      "0d0fd12bcbbbe6465b96ad8e111042560de47aad79bd1473c882bad2fce71b4c78a3"
      "b99e4f9b405e84fddd6815c6c4059cfb4b896880fb01e01fee33c9113a075c110782"
      "4b342545f984d90d0cd29e951dd482ec068a6c4fbc02f86ba5ca1bda0d847972287e"
      "6715cf5975d135c14bc8a9703b1aa6294c44a50979d0d789fa997a41f00899002a0f"
      "2928b6bf2b57dda37ea5d38d06e12734b88f566e5ac6d9be1f48afa815d7f2f0841d"
      "3177105b348379525fcc9a6a5fd14953ca3947ca43cde56eabf0be2867679a632c29"
      "1b8354fa0e4a0f9ba5ba2f2122c4e31d2001c0ea858b466dcc3c1c45cf8b19392073"
      "a45ed261f64878021d18f43520238d0205dee2cd3e6ef878d407f5f5cb3832204f4b"
      "80ee92174bf3f470c6e1800499d8d16f03c0d2946c460127a873dc46021ceeb84c34"
      "d136efdfdce8283daf3aeab90aa4f1256d5509838c96349cb88f1b817c94177e8927"
      "1f3c464b7dab45868a80f52740ec504334b87b1ffe7315d13e76ea8379873b89c652"
      "e7ac0984475231e71c67144a0c59924f96dc800d09b00a3a27f86f62f79317f7fc43"
      "9a5e235fce80908a60256543b5f3564a8205500b26e4a7d35bf60251ad8d52c8d44a"
      "6e3d0b111aaccbd813f1aa042b554b911bd3385e26be1af29cd0d3afa204e922a2ee"
      "039875df3fdb5c65636939a186304f9e3095bb82cba060b043e138f36ff275e00ca2"
      "7ec7a57a91853f19ee520d9a3a9e1ab97d57683e6091229180b6167d566982ef0865"
      "a70e487e27116d7ca227e7500765d4ca712cffd73dea70f138714221f308548c2fe9"
      "697a5a8204cb00dab455223b21dbda4ace383b4c2d5cb8a80b43a1cf80a85bcdbae4"
      "43e7cc65931b7d9c395a09d85aa04a0b896a1ababc3de81b258f22af403b0e45923c"
      "8ef7d2f68aa224aeb1261cfa4f645649c5f9597037eebfc066e517495d983e1f0461"
      "c651b4e425aca81ac92f7d624688b615d61b0dc20bacf56710803c549ebf9a558f65"
      "2ba4052026c5725e0f17a016eee0e070c1a857b8bd6d920397072b5cda654653a4ae"
      "18167007127ede81aea309b7a226293309449c3df4bb06bcfeab20b9fd0f7a682a55"
      "7269bfb1030c7d87fa7337fec56a7126e553a65869b94b8d41925d9fe4ca15de3b91"
      "6d497206bf80fbd657658c3cd59efe2d3e16f9886d48eb21ac931e9831fef5e1d8b0"
      "c28b0935378da0ab01455fb9e7e0305b638995f6dc7c29cde4293f871fd0318419a5"
      "fd289a1dbc78bff788fe84c5d85b0eabde25192665f1a9611dafc364dd2c76548e56"
      "c0c09f33ddcddacdb858e689ef60bc9024069c836cd9231a25c204a450c7a141ca2b"
      "6283b46f39aab490f368d5ecd8dbc2856ef94d48106550406fce98f9cac770369486"
      "9d9e9d9d56adadd6e5d7938503fed3e7f11f13ec55c87f9582e6b6b803f4359e4f1b"
      "1f0fd64137140da024169208957ec32301644b68fea7730fe73b03be378ab66d6ba0"
      "d9a95f44572e577ce0af4d5b169d0f5262e70a8056d0e9c9aa8c7e199b0853126de5"
      "75fcba6c7157789f46cd81870369218875e27ed132b80688035798300e01d621ce19"
      "a80a712a152d04eb4a1329eef9c2a76b3d8ef87aa5a4e5a0df0b5d86b5f6cf2a30a1"
      "4e0eea06fc7bbd9f31bb761e527b1a302a709e048db0829bd834cfc38ff0d8656eaf"
      "35dde2948b9001848b61ca80842be9eb2d821f4968e4a5ba17c407d0614e305b4867"
      "eb10f6fe0fde52c620f171ee0e8431d95c2fe6ebea9dd3edef4e38e1c06a447f238a"
      "aeb00aeb3a2deeaf11def8c70306e30f361000bbab84bd2b8d00f7489e11cef6ccf2"
      "2b2f73f5df1bd8ceaa96c72c2f73479ef034457bd48f180f8c25885f7317578934ad"
      "b4d88d0b9e0716f011c7ca6bdba21d9a2287bba225bdac9ff40456ecfe0c110dd2a1"
      "d02ed119a5219b5532def6e59da498dd52fd0450fc239de044a71e773c66435a1abb"
      "5a717ddb44bf0bfe38e606f3eebf545a6932428ac4e03891d6d124f97eb62b06e3ba"
      "4dd1c881a96de13e30a1dd4f609568cf3509c4aeb66df6ba0aa368ba6840f117e499"
      "d1a2db7f300623ed3fadde6513cd414a8767a153f8e514260e5fddc12f74cd7b2abe"
      "5a8a12268d3e0f0ce4aeb1cb02619795b7a9564f04bf7a66751f45b8c55581fdbb1d"
      "ef6bec2bc591361e6c5f655c154bebe62bda03d0cc13781351a63834817768044125"
      "204b2ccbe4a34ad8787913d19c51c5fe119692ad88701e41f410d2e551b4b4485cc6"
      "104ff8f3a96e60d441b981914b6f3a6e0a103874f5c8b5279c822380e1388f8c4e05"
      "1806df43d088919b6a96651a1fd18978d9f7b7303d81086fc802e80dbacddca3c67f"
      "75c31f9296a6a15671b828ffa2144d7d28aff2ef13981c08f29ceeeae61a4abf670a"
      "177bb66ee9eab7ba051fcea86c57279c74792c0db9d7be07b568a29be798e5cabe3f"
      "5922cb9bfe291d5cc733bedd48be1e0b42580ce82b2d192bc91cf834eed96e264049"
      "3f452eb90eebfedecbb6eef637fdfb36c58fbbf19f34d15c965e67f1bd19f8a86954"
      "190813ec1f32f9858ad282f3c3bb9e6599a60ff5a450bbee8a0fda246f1d98ec49f7"
      "1d3dddd8a3b894123279820f5e5eab5ee35be19ccdba7484205dac689e5af43f30de"
      "1eea0415c1d5844265ceb119755aab460fbb69450d1385e0917389dd2a8a254c9e11"
      "779fd80912f282c97cda530d1802588e37c57ebbc0fdd6b34afcd00008e3f2860c02"
      "9e87bd0c9cf9b7c875c78138e181c6b82180f1c2a324157dc1821a868d1fe05337df"
      "91051b0a712662f8174fd1363548e629e1a7ac2468ef99a4249640812f0cc2793c33"
      "0e7ae5d8e6f58ba598a75d378dd30fc43825e549382f0998cc63a91bd59562fc0194"
      "63e2ee0288ce8b972d02bfd6931c71e96f1438941f73a9fb0645e21bd830afa898cf"
      "adeae7092dc212ba3af1bfede863ac1868072674b962c28f9ffcc7c0b41eb3ce3f9b"
      "233bfeb7c73c1af34eb770f211e36d213d1327c5e115ec13741598df6924ecd5c49d"
      "92198e0690e5853163868077885525e2149476914ecd2da886cca521d9d4326fd076"
      "7d4760e02ebd300cd94f158439b3bce3e80daada688d0d82201e03704a0b7e817b82"
      "2ff42671ab1178c0fbef25872f62408a9918696ebfaa016ac83ab0ae6d7e564e4dab"
      "9cdf72c2be75e11a2e3bd421fab646c18bffe2b414db0ae47a2178b1fedb66099427"
      "7df443daba0335987650196c1459fec34300902fa7e093a319815ae9ca77a05e36cf"
      "c6ca1e921d38e5ce1d435f0780add12a7866d0e09039ee77696fa71b98b2ddf7cda6"
      "08aa3d711b36c0d7b7f00562173c3b1fff6d4b8bda89b59946a0702a1ec432d3369e"
      "03707faa4c8e40725e3a93b1b3250990aba031b44511c2427f69e82b75f5ebdf3936"
      "5ef9785786c9db0219b79f2520478d52490d3cd7f8c369f82ebd7401588c282ee868"
      "d6a578cc55fe5bbe83f32ea45b1da7169669275ab8de98ff73a74296359fcfba3a8b"
      "0fc1ba5c077096896e30077237231fb2573b4b564fb2f4eaab2b1c23da8f350ae14b"
      "bed8724bf9f0a99634fbfb703448d30d0e23df250781c655397703676e87a1c19498"
      "8c4c986c72fda703fb3a3e6147e35a50e82780b7299218452340cf7493fc24fedf14"
      "5ccdc6db06dd4d5bfd42cb9651b9530109d4678f110beb595c0d34e894bcd2fed27f"
      "0d5bd494852591ad791987f09f1484a4d5040529435f83c2ca21a450f450eb7641c8"
      "555a7ad97932d2bf415190e91ca48a9d2ad7c5f66198b40c33f2319aa0f877081061"
      "399259a141abe5b8501e3e4597960d65f6e1d3779cbc2f2962d86cfc3a791b8f7b81"
      "9fa9eac682de213ed1fe66ed39f7f5be6973d00ee63c9d6d7ebcd4a418b2ba25eb77"
      "90f8a9be0849c254e7123d05f8a95a277ab53929d5b270349b51fdd4b00cdb7f9084"
      "3da55e01544592f637c94a736de566ce1febb0f7ac8996b0b5f8d2681ead3b9a2070"
      "5c79793445fc22f993a1e80a5cef83bd897dc24974e9dd779d6f8a532f614a6886e4"
      "24eef1be0a2b93fffd2a0224f8efbc65306d275419a42395fed06402da279a57cf71"
      "27b6171548af2ec8c14000eca0ac27c16779a6f75600121b7461f4c9df3c10e33b64"
      "9f5b4cb450c9d18fdd199b1817f092f4709e33e3bcdcbb5ab10345568bcb4a753718"
      "7e851110074a356ac21e4bc7456b5c9039a0a8cc82655a5ad404d0a8aeca1f7c6432"
      "972230d605468c674b86e1ef0e84677733db6d44ade25808d3802ab1114b56e553bc"
      "8ae55f75fc61ce203ad84726396eb136932b66d0c948ee67929c1abfedefe6b7d679"
      "9743e0d6971f518315652d5ef0b780da21fbf4b99d543d1d122ceac03d3e08584d69"
      "2aedba5250387f71159efcf2120779c907bb358badcf9f72bf7817cf8f5936c19a51"
      "59cdecbabd6a7954dc46ed2dd50b58264f823b8dbfcb37e5a5fc8d418d9e144fee2a"
      "4bd1dec0ad923ab884f229e3aa247011f2c78f46c15ceacb0cd5b579d425ab947841"
      "1df734ed88962b8ba32abd9a4e7e02299736dc2c4c6303c80bb434a062461eb7b9fa"
      "7dced4e993427744488fa96d03cf019230ddafcc0d32a14c2808eff7dfb87a5088c0"
      "dd3a838394b696b33d8b5d9823923a58ad90a24582ea95485ea91296e22c646448a4"
      "baf64ab5b2f831aee378ad5e05444029f3b885160e2790ab96566f008dc845829219"
      "51e0ee071b51a5c1ab2b81d1e30e2b2511ab57d85413895fb33b6271f742b5d2a14c"
      "4ebbbdbcb1f8a7ba0e6efef4d95c1ac0d6e400a96e468e6ab948e73d8dae59e30ff3"
      "9742598d3c64cada3a37d590784368d3db26cdbaa3185f8085a8264b10ccbc43ec9f"
      "6eeb363f006f169f0aa4168acde5599e16b161b885761f0c9dcfd2b33fdbbe86f2a8"
      "8bef56fc46dfb075e01c84304ad1ea44aca68032de60acc3875e4b41630674ca07a9"
      "41d72ec4c212e885edd4a7ae77910059d344fd0545cdfb4170b2b735d575a56d743f"
      "c7e46dad742b4dfdd228cd30b2f49e86761f6c57f6d292a040444817b74d341127d5"
      "ca13ed730fc558f182b994a37565f0e05765df6004e7f288a54f311003c965b06dd4"
      "1de117f54d41a0482d3276fe3ed5535b228e95c12135b7d190b11c4f56fb77f1136e"
      "c0d76acf538abdd7d8fccfe0d69d23c46942283ec4c996b87e421d8219b3fa486d6d"
      "0fd83577b8e58f222d352865df77f25515eed1c8bdf7c7bf97eee9ac8e4b7ab35342"
      "cab4a96ad1be12e936a048e20c9c86a3281a32663f1552ce084cecf9cb1bcbfc8cff"
      "5a90042be4bf8eeeef16db1ba48f2c04998b9a4e896405f50d5e1ba8a4c490bcdf07"
      "187b90940bc36f973671d349aa772ffe6e708866f97ea7af8fb582e6031c12c999a4"
      "8fa092af5b5c18690a2577c58ac5447b20ab0413c6620b3928ece273a1d107e78b5e"
      "2f3a7489e1b871685814048f5dd9f03771ccca4fe9f8d717687a13c6b6a144441798"
      "5815f3a850fb0f6d36a1ae176b49a4a154fdc436821c5d6b1eca1012c46d5c6458fc"
      "a872a2ad3e12befaab636abd61bf440b5d3decbfead9ac3ea4a8f6342fa4dbb66340"
      "1836c67a126fe16fb35eee5104ee9d4b2da52d52e42d62ad866286a32305904e1f26"
      "7f3b0e71696b8eb1892a2bf0bbf0bc86abf6144d12c3f90a110d69766b14dcc06f8c"
      "20b9ef4bfa181390c2a78cc3b72c3687caaa7f3e4135dccb63cd2467d49af76784f6"
      "ace55bd2bc12bca438a4d99807e91d5d1571742922b099a46ebdea8f161720",
      ToHex(ZCashSerializer::SerializeRawTransaction(tx)));
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet
