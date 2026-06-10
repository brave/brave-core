/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

bool IsAddressAllowed(const std::string&) {
  return true;
}

}  // namespace

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

namespace {
void AppendMerklePath(OrchardNoteWitness& witness, const std::string& hex) {
  OrchardMerkleHash hash;
  base::span(hash).copy_from(*PrefixedHexStringToBytes(hex));
  witness.merkle_path.push_back(hash);
}
}  // namespace

// https://blockexplorer.one/zcash/testnet/tx/496dfffff625ada462cbc8a733f305fdef1ca584ceb8e7efa5e28e38249b466e
TEST(ZCashSerializerTest, OrchardToTransparentBundle) {
#if BUILDFLAG(IS_IOS)
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}}},
       {features::kBraveWalletWebUIFeature, {}}},
      {}  // disabled features
  );
#endif
  OrchardBundleManager::OverrideRandomSeedForTesting(6675565u);

  ZCashKeyring keyring(
      PrefixedHexStringToBytes(
          "0xe2c0aa2746fc727734c3beec18493053ca3e624fc6d4c4bffbcd7d56ae0f12c864"
          "70226552ba119ecb4de091bf51bc77ba22e1bd264af84ff5da575029edeab9")
          .value(),
      mojom::KeyringId::kZCashTestnet, base::BindRepeating(IsAddressAllowed));

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
        ZCashAddressToScriptPubkey("tmFDtouv8PyCYZjky6Y4z7Y67fXKka82bA9", true)
            .value();
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
      "0xb7f7080426736db2435919601f9167eada09b626776328fc1613078843588a70",
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
      "dc1811c7211ce6c98718420f72d3f428e869a31b445e3ab0816c79c7661f7cc8bcd98399"
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
      "773af7db831b7b5f85c85f1831ce75735a8b9b39c31b8d1b5cfd996daca41709d4fc217d"
      "fe1a03f08a76d89a4bd36f7e7a1a6dc4bfe6432cbbc127f219d7711225919ef2a98188e7"
      "e1b268d68d76168819a7e40ef0d9fcc75aafbf57e7f70f3972d228a69c6e4d127a67733b"
      "0f11caea37185562822cffc1502bdb1a044e9cb7bdd30c1bf49b8636f0a41dab6e82c23c"
      "7277de1fbcf7d69acb0cce21ffd29a08e20d851de63de3b7d5fd9c1b0ea21f208891905e"
      "706afc79e13008acc5625adadb94cefa913974699c691b5dd78162d0b96ae32d9755f029"
      "de1ad4af996fd91cedad3b96df37553aa9aafee8053ddbf14e4473aa80b2b46d0f1f6efe"
      "dd849db82f7c5b9dd1ab3ba4c0fc73bb3854ae3ed140932cbfa1e3ac479da9e5178a1a0c"
      "9388e360b6352a8d793350ba0043d70f769a8c6af945974ba0a68c905d8ba86061eb25e9"
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
      "acf8b4096cd31002e6ac4e9437c068b065709780bf05f398e7e5526414c10214216dca06"
      "14e03c085c522bec3704f60ac077496ac90162f9cd101f8c41dcacc31f0db6d063de96c7"
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
      "86516fb47eac61e78ac3d1b29caa12b5b84dfd1088f39a603f53a8ec326a5aa0c2bc8c6d"
      "513ae5ba668c6e089d3cb95840979cac2d4fe06be551d60164b041828d212d55488809df"
      "dc48ac712614130d3a9387a0984f8485b42fe55325a9c711a3bfab98ef8580121318b24b"
      "129dd8e35bdca00b25fd79abc3035817ff881da0f51346c92292b49846f091e5bc267cf0"
      "11e7bbcef005cd08b5bb21c6ef7c511d7d9bcb8aa5c5b2a6afeb3e86570126976a32c652"
      "90b8e04f60f91b5b8fb1839d99bc08171aa731da67da3e837a07ffe01c9b31b719429479"
      "0a5b88097e049dcdc8cb3a713637e3af447824e20ba9a9ad28629727d60b0e95c07a57d5"
      "9c02ebd135143d9fd22ded68279e40a7a9aef0cd5f40877e2e3f459e8488a9e5426204bd"
      "32cd030dcf195b953872b7d23abcb4cd429093348e204cdb8f28eb96bdcb6555143bf74c"
      "50ce5f63f36ff7aa40269e808c3c02cdc3ced31172b2f013398827e7620b3871f0904821"
      "5be82b87a83597ec3938c634b83c4b6a0a91515f375b350fcf0f838beb3f3c6c0c94a3ac"
      "b1906e3bae78b866fa9f8628fbc8820aa5490dc54f55cb88869ce07807cc7cef74266eaf"
      "6461f77ce0ea0b74ef8fdfbdc7078888e14327af5e96fdca46f880f5be2757b2a1e1a279"
      "c8d1242d9607948ca85fc64bab1a4f481dfe07949e41cf508f916033143cf3aaf9739d48"
      "3a7049dcbb2cf20e5a3949a21daac8412af5e62c63194fe391aea5f380ca0c6251622b1a"
      "652a8b9657c76bc814906cfcd3bc599b2da3c008bda5ca9de75877d3e96659d613c1725a"
      "01032b3bf2381b437a4e441fdcb464203e35a41e9064eabd7e9cf783834cb7f8c927cd59"
      "e5bdfb3727cb656e15ba975f37ece69003b3d0acf1af9c95b685f29245bb3c343bb4de31"
      "34eecefe571a5fddde64cc616bdbf45f9091bbab20c3f3591541b00a1ade5b33a3899bb2"
      "398577ea81a5e138b191bdb975eae26f79eb97df42a92654de7095c22264947aa70e4b55"
      "4690badb1264eb589c4a30c5f2602fdb0da081bc02c460a4a4e7d7bdb09ce3a69c701341"
      "de2fce70ec48a26e876c2c21d1ce87435e8a8a0efc84a8cb93ad80525427d0e0479a278d"
      "eca3b1263b9e79fc629357a40490194bebe2311d6b0f050d7c431f97989b0e21cf64e3f4"
      "d06190f81724a017aca5e7a8bd5dc16e46bdd0b39ad7d0fb02870fe2f73137125b92ed61"
      "80f9806046164566b89f567ad49df7e03a4adb7aeaf6e277188e2e165892f80d405ebe95"
      "37cf882278fba330f9bf0645b607768108d432e8920b3e27cdd76cf0c0053cbfe29caa41"
      "9c562d521a2aa56bdb8d28afbcc73d3cfdc602b088616d361a65b8130f1a70e8dea2f238"
      "97a86b10ba8cacae3b257622df6ba7262b39b6e58dbe47e1fae2ab8148edf8c4a39bd0a0"
      "a5d8569575e46f2dad5321e2bbb3468533314017960a1345d14eebe1fa18241d936c2596"
      "172a84360f8ee67c5cbcb4e86062a21ffad60e0d48f8315a97100895c24db157d08d0f0d"
      "8b558217ed92f6ee636284f8a53b82993c2c89f7aa1aa4755ca4d60c1d9f777d8f7ed2b8"
      "d53339a7e9919b95c2080fbfcb20553d0512e42529d9aa33f3d17a8536d8a2823682bad6"
      "39e02fcb9ec8bdcc22b730a69133cbf665c30a547def1166964c91b50d74582593059447"
      "95ec1b116c3b4c8dd50ced75449eb46a49d1f3be5e67b46da0be590faae579ec632e3850"
      "bebad35d0430554c3acb13c4d17c181a91db2b188e2ca3e86c584e31c6c75461f0a82bb9"
      "5617f86f3db0bf16d1c4d89d5086400200d05e1cabffa84a89599120bb9cd5349725041b"
      "6cab4c01fb68e31cf7eb50a616a5a713a07707314f7a8abf40f505a821150d7780b764f9"
      "6e47ecee371c4e71d27c1a9071f8555be84c5377f66adfc2da3b695fa3f6fcfe65f18e19"
      "1f471016bbaf56574a7af668cfc6d5d01a0389128d250de54e7de556a299960b3f8ba5e5"
      "eac9cf93fedd7196750e107a5d9b9d2dee142d27fcb6f4c66b9ab5dd2ea74415a61ec99b"
      "1b1840879459efce11285df58e09c6124cfe239c67fefcd64ebbc893c84618ab45be4eea"
      "abf06f0aa1b92069fc27127b525a597a02c894b8a7c6809453637b8049f6ddd0cb93e2b8"
      "029180c0cf1ac7481955be97fa4cc8e3e95030c4017da5ccb16045cfbfb0d0a94dd7c379"
      "e533e1abdfc51c04de65385bc0760995626a60f18a4810dfbd660ad9281eeef48f1e575e"
      "c3d35658646ee5e8b9a730b2a6c7aeb5cea445e987dc0adcd27ccbbfb432dd9a22ef9587"
      "d0145b49a337050376b522d49da57c77e55e875fcb04362c6010029721b68928a5a19b97"
      "868b8865eebb4739172c25fbbd7455996caa50fe2f29ce067fb87fcac3d4aae63cadd09f"
      "c7f32f51bea4a5f4f23c5a112dadc31fbf13dbcd9ae83803d0715a75938ca3ca51868f3e"
      "dfed8714825470acbe33ec32023a718ad80f3ad8cc6a0a5ff4837a4fab514bd22c9749cd"
      "a05dff1f64c659170627f7cb3eea2da2f95591f88867ea4aea14b1992ef60383df2440db"
      "37e5fd69d9061ace4b5efc500bab0a57cdeaafa0cd6ad4ce907f06678511dd3a82d19077"
      "ab2f1f7fe9716e0b0cebfa71780956fa4d9759c6a3a6456e381dd2238ea767f8362867ee"
      "630e60d71087d2e57e0d1f94e02da5a577467a60fc6ced18426aeded5f1dd67c2743e973"
      "9513ea39d567a2e5b86a8fee5c89ca03c5deefc9382363c34a12650396c5c42238ccb7d0"
      "f50dd362731ac4aba790acec998fcd7d58185afd4f374e9b3decba1ca0a8ba1e8c4b00df"
      "73c1e09354bde8f3762043750b55fc252608b6349c644ce4e8dcd862b5ce77853356d312"
      "f7ea1cee2893e73a9da76b874a3b22de55903b6c44680220ffcb202143936dfbfd5e715a"
      "f554959f3c967fbf7e046e6d87518c7b8d399967bfd9048156cac4f9ab0e396315c67a0a"
      "2f1dd1b6b902748f89984ca67e607f044b115f08c031810f2a25c7f278d699e3a082f65b"
      "51111d7501df3d0cfa51c48fbad4beff6a4ff134254c71a606594ecbc69c137b02137855"
      "b8cd98f937a498783191f17a4367163691a7f65d4bedfa0c11d2b7605b3374bd17930e19"
      "30ba088077995f126f6ae8c61eab513c9f0f6edaa48cc2bca10b2450d436324dce7fb13f"
      "c93b601d31f66323bed699cc9ed4f4056772a8c3f10f6511aba1dca3b12071787ed92eaa"
      "2e260ee0e5fe86173c87d2a63f66d6d38510ccb7d208458e57c07597f12fc8e3d7b0413d"
      "0ec6087484fb7d5f3645aa35a638e2c585dec5da6cb911158c24076c9fa0d7d793c2f620"
      "87f56db3af7f6502992aa0fecf678dd7420cabf02eacdc14785d9cfd6a912532eb09edd0"
      "99d0f9e3ff0a68b8c93a2eb52d7f1f7ee548523139ee6689d313e7ecaaace76c7247756c"
      "ec278f5a9404ae6c919a667ca0719d5ba7aaea7ee403d5cff0fc9ecf9227f1b91714bcc7"
      "26b88345b6824bd15eed94ce63f49e7577d34683427d07a934bc6340b01f4c6c9115aaa2"
      "15095dd46a34c2ac946547965ccef7ad26250862385224702c18ef08af70e1e7e10cabc1"
      "68f182baaf45d0b474773c050f548a27de10a73370121e68b5a2eccb06d7d7b0716c540a"
      "b9183effc9f2cbc8ff135de16f0ce14ad331392c4d11676c950e8d72a5b9005e15ac9c13"
      "4149bb15bcbfca21489957c5c2214b4cbb1245dff2b40dedc517fe9943b2375406cb885d"
      "11279f02179ab8d7d214d29c99155963068d05788f770e3316b4ba13c228db4493f4846c"
      "2f3f31fee533726eb011500c3155a688c3fe940b3dbf80f01ff641437692cee24063f321"
      "6c213bf595cc5df984001b516aa88531b1d8e9c00cbb0635d7e66d90f48f7228c41be5bb"
      "5cb8428fac5c5f29619d477c62ec5437b59fe9edc525734588925a0c4d16330b14edb1c0"
      "9d9b237d5f2a554c7863a0d2a793446be0e3dd5b53eb67f9253ec1141288b4eedc6de4ac"
      "8663b15390017799d85c496935af695825cb0c711f128586ba7ddfaf7088838637a21b43"
      "29e99efc9ebcffe6a82a97cc978a65375e03c7fdf2474c8d7d710244972de81f8d6e20b5"
      "97eb99a92e2bf4482163c385b12f0d066a2f828ab9e0a41456684e35ccc1260a19ce612a"
      "7717142376500be3e83b7769339f7131d0eba52d2275bc67f8c3b0d16a1ebe95c3a340c7"
      "a8ee024beb023c0a9c510fcf6b136acffdde47bb232d3bdfe474a91820961b3a2f2e833c"
      "e80f7bff6b7e0f557085a6ac978e9914fab50ea2505c2fabcc0ba538e8ca2d554b272cba"
      "470e23c1c9861a352211565117fcb325be64261ba02b4604cfa10402fd29841142b855c3"
      "05085d70783e6bada8401758852b3bd2f977e2f8f0f45caf4526b3320ec9bf78ad3e6b12"
      "d677c1412513d325c7c19140e84bc470d626dfc2a01d71bbf88bc43834fa1deeb11ff4ba"
      "42f577d09f889fcb4bbfbc35fc6b9a8c730ec75f3404dbdefdb05b4dd915123987c2753e"
      "08e4cb6cdb954231c7ed49c9ae060011b5d023c5bf6148576299d5f47e280fc1bd6caa3f"
      "ac130f514d285eaeca18305e5e4894270b0b495593c8d439d5ee2dd3b94627f4f95c0acf"
      "202785ecc5376d18a79ffd00d3377c2d31588b48b49b3c9abcaf391cf2e2218996c76db2"
      "8619bf7bf968d3e6f04d3add20d4f70967fc8833265ca7e2b7736192b2c8bd44ed39c05e"
      "5fb5ea298b65037bfd4ca3c5e753f09c15e4046424f5c56545dcbc62b838c55aea8c81f9"
      "fbca369c4377df1b6b0917afa541e692d5eab6b97b3e6cf656310ce1d23b2544a5e1883e"
      "775ca6f6b6471b1e70c6011a2e9725ec315b866a142a759cb1e90abbf76723d992610b08"
      "a18b3848e9896ec43b80e2242e87a5dfaa1c71e8dd5141ee5667967e08ac1dd0b91cb636"
      "b4b91452dda1d0c71e11f4f49a185de4e7ff3b57ab67c8b34312ae8b2e434619b59da2a9"
      "58b47ef8ed77602a9f2ce672c509210dba395ce2c7034741380b8c5f52b49dcd2886d0bb"
      "a0e60c7aa00532af17ad687a75d4baed7b86586e9168fe5aef85cd4007763211bd140eb3"
      "08183accbda597d9c948a5e4171e6f2ade9a0679fac7f7d190dec84eb9d70a8e49371d2d"
      "bff9fd828d1e766dc43a2872db9fe21e15e6ddc89f22fe4db7a29672ee2fcc661868ccd7"
      "0ef44d5ab2993479f4e74af16ab6a4cea860ccdf04b9a922d537c83d28b94612d5f5105b"
      "b37b684454b280fa186c124b2f0988660b2258921e3c6ebd8354e219e92a28ec0a762f63"
      "caccaafe96a9a1148de16e4eec41a346121a73d9ced492de426ee67332d98ec521b58870"
      "76b07f57fdc7aa3b7c810f207b2c7fca3959eee1a1a35576a9982c3b4341daef99c35049"
      "c05ae4268af5291fc516faab852bd00d1854858c943ec7994b39990167ee9401fe682e39"
      "82ae4f41513da7bc320e803e81b223219bcb1470b59bdf939b2c55eb9be23c13350294c4"
      "f31eaf4b76603e9fc50da8f1cf5b05cf9126fe21c0cc51959c3c2490b638c8ed050d10ea"
      "3a2c1ec1b128835982724c129cfb79d67c51d39fc2f26888ccfe753f7f36f4ceba8596ab"
      "d313a21e56b327915ddb6d657fdbcf40770b21a708908d02a83d3fe7645558f0ee24fda5"
      "2da89ed9b4e3b8c863d1372867ae9a4aa7cd74b8190117f71c0bde711d347556646a57f7"
      "46c85cbf4542fa26adaaef5efd5ac3e5af0e9c78dbb4b112e984b5001421240f0a7051de"
      "5bc123eea043ce5c4fd2b84c890a8224d8b2a2bc3e46003f4d1722e200c3910d572ea168"
      "e023d16c2fcf7b77cb223540bd2c2f47018b22fd2001065cd0ac09ff698badc438fe48ae"
      "8bddf379553a655e81706b44c53d964fffb1333370d59d759babd72c952a59d93a395cdf"
      "cd3ed30253e0c8940701e32f6b3a8e39d127a2b4073ec35784d7c87e3e4fb60a7708e216"
      "2dc5fd3a1befd17a27c7c90148bc5af195882ca1f3c63634406851ec251e993776cb7717"
      "36be958091e24fa818273a8d548fcfe6b7ce82debc389fc4b126439ca43fff7d911945e5"
      "4d4f895d9c4b28c7d50c1bfad7a5a24abaf35fc17121d785098956af5e24e9f1b6f9104f"
      "08d1db68694ca2353eb8c85675d6dc31233c8b31df9e667e628c9c61eba4cbb5716bb8d5"
      "cc49aef69feb8e085cb9a5a11c15ae75477d9adba2f67954c1bd5d2eb53528926e3cfaa1"
      "1d3ad1b7bd8c21d7322907486988f374e24dce34756dd784c0a3e02bb955b33e88b66ad6"
      "2e6e23bdb00c46fdec627d1419cceae877ee4b1b708e04380404bd0588152e5e74787792"
      "cc26a228b653135415a2329009bf9dcebf2ef8b1285c19575cc2a28bc99c6d9616172c7f"
      "aa5358976be44fe78c9a897e9271323eaa5731daa2ed3587487cc9046b305567f5751cb3"
      "e6e5a018cc7636b5e8989e6a0d5803e1ece7c86d962accf5ec3bd277a800548eef43349b"
      "ab830ba2abf0daf11f5c4ea740f72fb0d087fd191f16453f9a1fb8ff07d6ab214bcb0416"
      "49192a08d4b2cd0d5408f72e74d4a1df27318972428ac4816a1e2a3c73f54877d5f7331e"
      "9b420e669abce21bdb80f2614b13bde5eb4775f9d03dd5b3f850c900988adbedc93e77dd"
      "641f12cc09bf639cf1270742095df8f7ca247e30cd93b77d245a2d54d43da6505506168f"
      "b631a93a672bd6dd2c5e8b56709d2295e24118a8bb501ba61b22b8e0b1cf6ab7f48bfbbe"
      "03049f19ef917062818cb1a42a6e5bf4af2caeffb3277e089258293ac3fc77915e1e28c0"
      "da6d20ed64c3aded8c9229cf4de2af597b04b4d8aa9cb79fd6c835043124852bf2c800c8"
      "68ebaab37cb5d353940df2e07151cd0288cb459d9b9df85731346a112a9738ffcce19614"
      "25b341fa6bb7d2eaf468e1b454871e48461d09405e26cf1b1cfaa909ff6d0ed540ca53a5"
      "b0c101d17eb55ed39e2943499ee3985c8b0ab1b88167d056d7b75e9a9e388cc9c59498ad"
      "9aacd09b81e74c3df25bc499f40f869c7cd1ccd8c76218af0b2c269110ede64147895bb3"
      "61426fab519591665e3954849fa823db5b2b80ca6bf11b70b64a3f33a3d18dd6c3ceb0cc"
      "3ec0b0948d09c602ffb6a26dfe256472b3ebf14c54a984f18b6a515478656d5a0a14ca54"
      "2c3b7fd54bcd0689b21b898007aedacb38bf4536e05887826f90b54077ef02273715a39e"
      "70b0043a0208bd094c8d929f3f242137624dfcafdd51e653ad69a1ea1c0d4b93f40bad53"
      "7386a4e579c162c0cc1dc8ca40ae8f6e1ac287c4652a32caf10697365bcdd2d1996f495f"
      "f5017afa915e44360f939d128f5c784a970a0ba8b0342374c8013aaef7aa9cf8cf36de16"
      "90bee5bd40053649ffb9f4308bcfc5e4e71b58661ff3aec0f582aec74988e2a8b8016a18"
      "593b66cc7b17a509cfa9dcd1180f9f34b9a899240d24c3f1b73061c87bde5ee4bff1cc7d"
      "4e198ae683d45c7bcf3b8f78ae32f55626b6486778ecb28e8fc3c5e17e83ebc3952d7a0f"
      "de4c66cbae3a32e9b01c092a7878a67da91f1510bd7a3d993a009b73e47e0cce3a23b332"
      "ea20fc485beae1188c15c3a41ed25d47719d1e889a0ccc9eefbdcceb4b7eeac9c20f7d8a"
      "541de3a2dafbd9820a7556b0d8db5f290ddbc4a5cb6e53d316ceeb79bf285abb6a6444e9"
      "a0a12e9ba02a6786068da49557c33c10a4d5515439b58b0bf030b19d89bfe59374e7f240"
      "8f922d4f03058240d9d1467670ac7d77faea506077164aca85cbeb9727d24b67430c68d5"
      "24cf49c577475aa16b8db0bbcc6665a3ab0f76f9bfccd5acd190ddfd4fcc91176cebaa3b"
      "11e5ee4f09932361b99096d345042169e21bcef3f267432c51ac86859d9d837ed40c0397"
      "f665016c16805710e81b86f5d47addcf06c3f955ad34a895f18a006dc1ab3a8bfef4ee38"
      "ac95e354862fed2eb47a7e63482b94a0d268aa8fabd4f4f8123e70d73eecfc83b0076a3b"
      "b70486da5acfece2a028a0f9756c071164e2706b5b61ed49b4fb9aefc0866a22f6052cf7"
      "fe51b52c58b9cca771f18ac068ee7b957eec4e159d37093cc8e38912b004beed4ba7f6a8"
      "eb7e87deff7fa027e513eb11b7315eebc1af06dec70bf5734d2169dcc9e56a8ff304873f"
      "64aa1fdbf056bdb527e7f8646f36947b97a89210740b1852fbcaf4eb971269cbf39dc14e"
      "a031115f07f8cb13e8de7a1e1f7fed6df22958307e5b949ee6a119269b29b35cd7fe1d80"
      "e905db9a4c0745e468a2a7402f07f1ec78a169b9e6b3ed45d16628b6767d7c2bc60c1456"
      "24c482bd6cf07b7e5421c8f08125c0a290c0885d60ab8fbbda3d5bd2ad57bfa7b8e9fcf2"
      "035442258a1e5671a6429d0997e93d4af222abcc93c3c88a3adb43122e457c5770e6eb1d"
      "d18ab867ebf8df2da7a97c369d31bf02c0842c9f6d4150f1f069c4d56f15e71267a271cf"
      "a3decd34d6f55f08c35ef40688453dd3bc787b3549d12b81354b6c770c9d0a7798c38848"
      "973fac8584a99652fafd7913792e20486c15716ed84901cec633505d8776c1a237796337"
      "13f109d0c752ccc4bc6f0b145d4d61694fef5dddbd1c268c1bb752c68d1e867f67094d8d"
      "48a68bb236763b52b1dab8f7146616d835378f102878ab0353eb467c3b0b8955b25a03ed"
      "808b3e76984d6a70b3e2bd3f061f3ca0b914f7971e222e6e671db09c292d9ddd5fd8cc3c"
      "6cbf3a58679e279a4e024aa91afa72c6600997547d8c2983ffbf2a1b9f74397db9769f00"
      "a27b2e2e842f996a9b9e6734e1fdd0ae7e5afe229efb099b9c5637d49fb79ae48126360e"
      "973a721e262814f561a79b1927113427d55c1c16076cd2477dda3547cf51dc0527968e2b"
      "42802399bc2246b70f6d458559e8ad1b6fe8719e3742690b4dc744a5971bc904d5b5d3a7"
      "7cba25aa512e5853c10ff1a2149b918c2b8cddfc2eca0102f23104fe459dc9b6edff5915"
      "0cce8da087a2823766c13c20cbc19800856eb0f08b1341189a5f2e4325b662d1afee4670"
      "1ae6d05b7153b10f35a7cb624d59a490dc9eb7890dcd2b4c04b65c9cb0a1ab988f6f5cc4"
      "322dc448c7d63dbf240bcbaeb3a06540ecb120027fa5e83063b641b49fcfbb7bd4c60d14"
      "0874458f59bb4f140dad5a659e2f3a9e45741f4fd291103bedd1c35f90fbd5984819dce1"
      "783a8223c9b16221e4104190a590167614b50576fce1d9ed11f75ab07b04de64d1edb1f9"
      "8b8b76711cdc46155c8f19fe26201a5eb1c570b15d285a3caab93b54b16fb1a021912f75"
      "7b7c217cc43ae068fb8729f26a58dd705f482754d23fa5c9eee2db4abe3e98461baf21a3"
      "d0b2a7ca3fb9bdbcaa79c3fa82f680218bd56037d8c1a7489c3e0fcd0929dbe8f4f75f55"
      "b46ee1928a44ed5d40a52ce95f15c2d252252c9881155ecfe6b13164ef169de8afee49a6"
      "d6806950763acd76bbc05965734209ffd5af42ce51c532e82bdd9c9d21c56457b5a06956"
      "5adece2ae11a0ab11083522baf2096c5b5c2761669e18821122055a4b0a1b7508aa30240"
      "af21f86f0dbd5a6992b25b305121f506cf24521f2fa4dd3a675109b59495937853b88951"
      "528c564a040a30b9db585d0c837a9a671f95483b96847e35759173a7dd157aec996dcb6a"
      "e91deb8787e12b393e48b96a5522dfc00d421000e5f7459efae0dff97f3cbd451d03");

  EXPECT_EQ(
      "0x050000800a27a726f04dec4d03f5370017f53700000100f2052a010000001976a9143c"
      "6ef7f4804a495040ad24bb5274c924ef70625788ac00000256fca85195acfc96859a47b2"
      "cff57aeb6eb71c84f60d2c5bdc37243baf8847a2c98823d9a80fa5b24c1945bef5ccfa66"
      "29ef3ef93dc367efa51a9b19269cb00045e6d46d7fdefd518b0c3b205d09439f044cba4c"
      "24aa3e8172ddb7647a0491367988a8f6d8f0b269fd173314f8256eb50e015756c1c7596c"
      "92f4daa956e7a31e8ec61d7d750c91cc8700a34c9edb975103c0bf06d7c4c29b1851739b"
      "1d865099c7a5aed5374f0ce22b76d136a80b4bda1354dc1811c7211ce6c98718420f72d3"
      "f428e869a31b445e3ab0816c79c7661f7cc8bcd9839987b61a33d28b5748900fa55d5c13"
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
      "7bd29b2e33ff8d1b176a1ad9c785d33809877329d511773af7db831b7b5f85c85f1831ce"
      "75735a8b9b39c31b8d1b5cfd996daca41709d4fc217dfe1a03f08a76d89a4bd36f7e7a1a"
      "6dc4bfe6432cbbc127f219d7711225919ef2a98188e7e1b268d68d76168819a7e40ef0d9"
      "fcc75aafbf57e7f70f3972d228a69c6e4d127a67733b0f11caea37185562822cffc1502b"
      "db1a044e9cb7bdd30c1bf49b8636f0a41dab6e82c23c7277de1fbcf7d69acb0cce21ffd2"
      "9a08e20d851de63de3b7d5fd9c1b0ea21f208891905e706afc79e13008acc5625adadb94"
      "cefa913974699c691b5dd78162d0b96ae32d9755f029de1ad4af996fd91cedad3b96df37"
      "553aa9aafee8053ddbf14e4473aa80b2b46d0f1f6efedd849db82f7c5b9dd1ab3ba4c0fc"
      "73bb3854ae3ed140932cbfa1e3ac479da9e5178a1a0c9388e360b6352a8d793350ba0043"
      "d70f769a8c6af945974ba0a68c905d8ba86061eb25e9a1f53313666cbad49a930493e1a3"
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
      "68b065709780bf05f398e7e5526414c10214216dca0614e03c085c522bec3704f60ac077"
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
      "12b5b84dfd1088f39a603f53a8ec326a5aa0c2bc8c6d513ae5ba668c6e089d3cb9584097"
      "9cac2d4fe06be551d60164b041828d212d55488809dfdc48ac712614130d3a9387a0984f"
      "8485b42fe55325a9c711a3bfab98ef8580121318b24b129dd8e35bdca00b25fd79abc303"
      "5817ff881da0f51346c92292b49846f091e5bc267cf011e7bbcef005cd08b5bb21c6ef7c"
      "511d7d9bcb8aa5c5b2a6afeb3e86570126976a32c65290b8e04f60f91b5b8fb1839d99bc"
      "08171aa731da67da3e837a07ffe01c9b31b7194294790a5b88097e049dcdc8cb3a713637"
      "e3af447824e20ba9a9ad28629727d60b0e95c07a57d59c02ebd135143d9fd22ded68279e"
      "40a7a9aef0cd5f40877e2e3f459e8488a9e5426204bd32cd030dcf195b953872b7d23abc"
      "b4cd429093348e204cdb8f28eb96bdcb6555143bf74c50ce5f63f36ff7aa40269e808c3c"
      "02cdc3ced31172b2f013398827e7620b3871f09048215be82b87a83597ec3938c634b83c"
      "4b6a0a91515f375b350fcf0f838beb3f3c6c0c94a3acb1906e3bae78b866fa9f8628fbc8"
      "820aa5490dc54f55cb88869ce07807cc7cef74266eaf6461f77ce0ea0b74ef8fdfbdc707"
      "8888e14327af5e96fdca46f880f5be2757b2a1e1a279c8d1242d9607948ca85fc64bab1a"
      "4f481dfe07949e41cf508f916033143cf3aaf9739d483a7049dcbb2cf20e5a3949a21daa"
      "c8412af5e62c63194fe391aea5f380ca0c6251622b1a652a8b9657c76bc814906cfcd3bc"
      "599b2da3c008bda5ca9de75877d3e96659d613c1725a01032b3bf2381b437a4e441fdcb4"
      "64203e35a41e9064eabd7e9cf783834cb7f8c927cd59e5bdfb3727cb656e15ba975f37ec"
      "e69003b3d0acf1af9c95b685f29245bb3c343bb4de3134eecefe571a5fddde64cc616bdb"
      "f45f9091bbab20c3f3591541b00a1ade5b33a3899bb2398577ea81a5e138b191bdb975ea"
      "e26f79eb97df42a92654de7095c22264947aa70e4b554690badb1264eb589c4a30c5f260"
      "2fdb0da081bc02c460a4a4e7d7bdb09ce3a69c701341de2fce70ec48a26e876c2c21d1ce"
      "87435e8a8a0efc84a8cb93ad80525427d0e0479a278deca3b1263b9e79fc629357a40490"
      "194bebe2311d6b0f050d7c431f97989b0e21cf64e3f4d06190f81724a017aca5e7a8bd5d"
      "c16e46bdd0b39ad7d0fb02870fe2f73137125b92ed6180f9806046164566b89f567ad49d"
      "f7e03a4adb7aeaf6e277188e2e165892f80d405ebe9537cf882278fba330f9bf0645b607"
      "768108d432e8920b3e27cdd76cf0c0053cbfe29caa419c562d521a2aa56bdb8d28afbcc7"
      "3d3cfdc602b088616d361a65b8130f1a70e8dea2f23897a86b10ba8cacae3b257622df6b"
      "a7262b39b6e58dbe47e1fae2ab8148edf8c4a39bd0a0a5d8569575e46f2dad5321e2bbb3"
      "468533314017960a1345d14eebe1fa18241d936c2596172a84360f8ee67c5cbcb4e86062"
      "a21ffad60e0d48f8315a97100895c24db157d08d0f0d8b558217ed92f6ee636284f8a53b"
      "82993c2c89f7aa1aa4755ca4d60c1d9f777d8f7ed2b8d53339a7e9919b95c2080fbfcb20"
      "553d0512e42529d9aa33f3d17a8536d8a2823682bad639e02fcb9ec8bdcc22b730a69133"
      "cbf665c30a547def1166964c91b50d7458259305944795ec1b116c3b4c8dd50ced75449e"
      "b46a49d1f3be5e67b46da0be590faae579ec632e3850bebad35d0430554c3acb13c4d17c"
      "181a91db2b188e2ca3e86c584e31c6c75461f0a82bb95617f86f3db0bf16d1c4d89d5086"
      "400200d05e1cabffa84a89599120bb9cd5349725041b6cab4c01fb68e31cf7eb50a616a5"
      "a713a07707314f7a8abf40f505a821150d7780b764f96e47ecee371c4e71d27c1a9071f8"
      "555be84c5377f66adfc2da3b695fa3f6fcfe65f18e191f471016bbaf56574a7af668cfc6"
      "d5d01a0389128d250de54e7de556a299960b3f8ba5e5eac9cf93fedd7196750e107a5d9b"
      "9d2dee142d27fcb6f4c66b9ab5dd2ea74415a61ec99b1b1840879459efce11285df58e09"
      "c6124cfe239c67fefcd64ebbc893c84618ab45be4eeaabf06f0aa1b92069fc27127b525a"
      "597a02c894b8a7c6809453637b8049f6ddd0cb93e2b8029180c0cf1ac7481955be97fa4c"
      "c8e3e95030c4017da5ccb16045cfbfb0d0a94dd7c379e533e1abdfc51c04de65385bc076"
      "0995626a60f18a4810dfbd660ad9281eeef48f1e575ec3d35658646ee5e8b9a730b2a6c7"
      "aeb5cea445e987dc0adcd27ccbbfb432dd9a22ef9587d0145b49a337050376b522d49da5"
      "7c77e55e875fcb04362c6010029721b68928a5a19b97868b8865eebb4739172c25fbbd74"
      "55996caa50fe2f29ce067fb87fcac3d4aae63cadd09fc7f32f51bea4a5f4f23c5a112dad"
      "c31fbf13dbcd9ae83803d0715a75938ca3ca51868f3edfed8714825470acbe33ec32023a"
      "718ad80f3ad8cc6a0a5ff4837a4fab514bd22c9749cda05dff1f64c659170627f7cb3eea"
      "2da2f95591f88867ea4aea14b1992ef60383df2440db37e5fd69d9061ace4b5efc500bab"
      "0a57cdeaafa0cd6ad4ce907f06678511dd3a82d19077ab2f1f7fe9716e0b0cebfa717809"
      "56fa4d9759c6a3a6456e381dd2238ea767f8362867ee630e60d71087d2e57e0d1f94e02d"
      "a5a577467a60fc6ced18426aeded5f1dd67c2743e9739513ea39d567a2e5b86a8fee5c89"
      "ca03c5deefc9382363c34a12650396c5c42238ccb7d0f50dd362731ac4aba790acec998f"
      "cd7d58185afd4f374e9b3decba1ca0a8ba1e8c4b00df73c1e09354bde8f3762043750b55"
      "fc252608b6349c644ce4e8dcd862b5ce77853356d312f7ea1cee2893e73a9da76b874a3b"
      "22de55903b6c44680220ffcb202143936dfbfd5e715af554959f3c967fbf7e046e6d8751"
      "8c7b8d399967bfd9048156cac4f9ab0e396315c67a0a2f1dd1b6b902748f89984ca67e60"
      "7f044b115f08c031810f2a25c7f278d699e3a082f65b51111d7501df3d0cfa51c48fbad4"
      "beff6a4ff134254c71a606594ecbc69c137b02137855b8cd98f937a498783191f17a4367"
      "163691a7f65d4bedfa0c11d2b7605b3374bd17930e1930ba088077995f126f6ae8c61eab"
      "513c9f0f6edaa48cc2bca10b2450d436324dce7fb13fc93b601d31f66323bed699cc9ed4"
      "f4056772a8c3f10f6511aba1dca3b12071787ed92eaa2e260ee0e5fe86173c87d2a63f66"
      "d6d38510ccb7d208458e57c07597f12fc8e3d7b0413d0ec6087484fb7d5f3645aa35a638"
      "e2c585dec5da6cb911158c24076c9fa0d7d793c2f62087f56db3af7f6502992aa0fecf67"
      "8dd7420cabf02eacdc14785d9cfd6a912532eb09edd099d0f9e3ff0a68b8c93a2eb52d7f"
      "1f7ee548523139ee6689d313e7ecaaace76c7247756cec278f5a9404ae6c919a667ca071"
      "9d5ba7aaea7ee403d5cff0fc9ecf9227f1b91714bcc726b88345b6824bd15eed94ce63f4"
      "9e7577d34683427d07a934bc6340b01f4c6c9115aaa215095dd46a34c2ac946547965cce"
      "f7ad26250862385224702c18ef08af70e1e7e10cabc168f182baaf45d0b474773c050f54"
      "8a27de10a73370121e68b5a2eccb06d7d7b0716c540ab9183effc9f2cbc8ff135de16f0c"
      "e14ad331392c4d11676c950e8d72a5b9005e15ac9c134149bb15bcbfca21489957c5c221"
      "4b4cbb1245dff2b40dedc517fe9943b2375406cb885d11279f02179ab8d7d214d29c9915"
      "5963068d05788f770e3316b4ba13c228db4493f4846c2f3f31fee533726eb011500c3155"
      "a688c3fe940b3dbf80f01ff641437692cee24063f3216c213bf595cc5df984001b516aa8"
      "8531b1d8e9c00cbb0635d7e66d90f48f7228c41be5bb5cb8428fac5c5f29619d477c62ec"
      "5437b59fe9edc525734588925a0c4d16330b14edb1c09d9b237d5f2a554c7863a0d2a793"
      "446be0e3dd5b53eb67f9253ec1141288b4eedc6de4ac8663b15390017799d85c496935af"
      "695825cb0c711f128586ba7ddfaf7088838637a21b4329e99efc9ebcffe6a82a97cc978a"
      "65375e03c7fdf2474c8d7d710244972de81f8d6e20b597eb99a92e2bf4482163c385b12f"
      "0d066a2f828ab9e0a41456684e35ccc1260a19ce612a7717142376500be3e83b7769339f"
      "7131d0eba52d2275bc67f8c3b0d16a1ebe95c3a340c7a8ee024beb023c0a9c510fcf6b13"
      "6acffdde47bb232d3bdfe474a91820961b3a2f2e833ce80f7bff6b7e0f557085a6ac978e"
      "9914fab50ea2505c2fabcc0ba538e8ca2d554b272cba470e23c1c9861a352211565117fc"
      "b325be64261ba02b4604cfa10402fd29841142b855c305085d70783e6bada8401758852b"
      "3bd2f977e2f8f0f45caf4526b3320ec9bf78ad3e6b12d677c1412513d325c7c19140e84b"
      "c470d626dfc2a01d71bbf88bc43834fa1deeb11ff4ba42f577d09f889fcb4bbfbc35fc6b"
      "9a8c730ec75f3404dbdefdb05b4dd915123987c2753e08e4cb6cdb954231c7ed49c9ae06"
      "0011b5d023c5bf6148576299d5f47e280fc1bd6caa3fac130f514d285eaeca18305e5e48"
      "94270b0b495593c8d439d5ee2dd3b94627f4f95c0acf202785ecc5376d18a79ffd00d337"
      "7c2d31588b48b49b3c9abcaf391cf2e2218996c76db28619bf7bf968d3e6f04d3add20d4"
      "f70967fc8833265ca7e2b7736192b2c8bd44ed39c05e5fb5ea298b65037bfd4ca3c5e753"
      "f09c15e4046424f5c56545dcbc62b838c55aea8c81f9fbca369c4377df1b6b0917afa541"
      "e692d5eab6b97b3e6cf656310ce1d23b2544a5e1883e775ca6f6b6471b1e70c6011a2e97"
      "25ec315b866a142a759cb1e90abbf76723d992610b08a18b3848e9896ec43b80e2242e87"
      "a5dfaa1c71e8dd5141ee5667967e08ac1dd0b91cb636b4b91452dda1d0c71e11f4f49a18"
      "5de4e7ff3b57ab67c8b34312ae8b2e434619b59da2a958b47ef8ed77602a9f2ce672c509"
      "210dba395ce2c7034741380b8c5f52b49dcd2886d0bba0e60c7aa00532af17ad687a75d4"
      "baed7b86586e9168fe5aef85cd4007763211bd140eb308183accbda597d9c948a5e4171e"
      "6f2ade9a0679fac7f7d190dec84eb9d70a8e49371d2dbff9fd828d1e766dc43a2872db9f"
      "e21e15e6ddc89f22fe4db7a29672ee2fcc661868ccd70ef44d5ab2993479f4e74af16ab6"
      "a4cea860ccdf04b9a922d537c83d28b94612d5f5105bb37b684454b280fa186c124b2f09"
      "88660b2258921e3c6ebd8354e219e92a28ec0a762f63caccaafe96a9a1148de16e4eec41"
      "a346121a73d9ced492de426ee67332d98ec521b5887076b07f57fdc7aa3b7c810f207b2c"
      "7fca3959eee1a1a35576a9982c3b4341daef99c35049c05ae4268af5291fc516faab852b"
      "d00d1854858c943ec7994b39990167ee9401fe682e3982ae4f41513da7bc320e803e81b2"
      "23219bcb1470b59bdf939b2c55eb9be23c13350294c4f31eaf4b76603e9fc50da8f1cf5b"
      "05cf9126fe21c0cc51959c3c2490b638c8ed050d10ea3a2c1ec1b128835982724c129cfb"
      "79d67c51d39fc2f26888ccfe753f7f36f4ceba8596abd313a21e56b327915ddb6d657fdb"
      "cf40770b21a708908d02a83d3fe7645558f0ee24fda52da89ed9b4e3b8c863d1372867ae"
      "9a4aa7cd74b8190117f71c0bde711d347556646a57f746c85cbf4542fa26adaaef5efd5a"
      "c3e5af0e9c78dbb4b112e984b5001421240f0a7051de5bc123eea043ce5c4fd2b84c890a"
      "8224d8b2a2bc3e46003f4d1722e200c3910d572ea168e023d16c2fcf7b77cb223540bd2c"
      "2f47018b22fd2001065cd0ac09ff698badc438fe48ae8bddf379553a655e81706b44c53d"
      "964fffb1333370d59d759babd72c952a59d93a395cdfcd3ed30253e0c8940701e32f6b3a"
      "8e39d127a2b4073ec35784d7c87e3e4fb60a7708e2162dc5fd3a1befd17a27c7c90148bc"
      "5af195882ca1f3c63634406851ec251e993776cb771736be958091e24fa818273a8d548f"
      "cfe6b7ce82debc389fc4b126439ca43fff7d911945e54d4f895d9c4b28c7d50c1bfad7a5"
      "a24abaf35fc17121d785098956af5e24e9f1b6f9104f08d1db68694ca2353eb8c85675d6"
      "dc31233c8b31df9e667e628c9c61eba4cbb5716bb8d5cc49aef69feb8e085cb9a5a11c15"
      "ae75477d9adba2f67954c1bd5d2eb53528926e3cfaa11d3ad1b7bd8c21d7322907486988"
      "f374e24dce34756dd784c0a3e02bb955b33e88b66ad62e6e23bdb00c46fdec627d1419cc"
      "eae877ee4b1b708e04380404bd0588152e5e74787792cc26a228b653135415a2329009bf"
      "9dcebf2ef8b1285c19575cc2a28bc99c6d9616172c7faa5358976be44fe78c9a897e9271"
      "323eaa5731daa2ed3587487cc9046b305567f5751cb3e6e5a018cc7636b5e8989e6a0d58"
      "03e1ece7c86d962accf5ec3bd277a800548eef43349bab830ba2abf0daf11f5c4ea740f7"
      "2fb0d087fd191f16453f9a1fb8ff07d6ab214bcb041649192a08d4b2cd0d5408f72e74d4"
      "a1df27318972428ac4816a1e2a3c73f54877d5f7331e9b420e669abce21bdb80f2614b13"
      "bde5eb4775f9d03dd5b3f850c900988adbedc93e77dd641f12cc09bf639cf1270742095d"
      "f8f7ca247e30cd93b77d245a2d54d43da6505506168fb631a93a672bd6dd2c5e8b56709d"
      "2295e24118a8bb501ba61b22b8e0b1cf6ab7f48bfbbe03049f19ef917062818cb1a42a6e"
      "5bf4af2caeffb3277e089258293ac3fc77915e1e28c0da6d20ed64c3aded8c9229cf4de2"
      "af597b04b4d8aa9cb79fd6c835043124852bf2c800c868ebaab37cb5d353940df2e07151"
      "cd0288cb459d9b9df85731346a112a9738ffcce1961425b341fa6bb7d2eaf468e1b45487"
      "1e48461d09405e26cf1b1cfaa909ff6d0ed540ca53a5b0c101d17eb55ed39e2943499ee3"
      "985c8b0ab1b88167d056d7b75e9a9e388cc9c59498ad9aacd09b81e74c3df25bc499f40f"
      "869c7cd1ccd8c76218af0b2c269110ede64147895bb361426fab519591665e3954849fa8"
      "23db5b2b80ca6bf11b70b64a3f33a3d18dd6c3ceb0cc3ec0b0948d09c602ffb6a26dfe25"
      "6472b3ebf14c54a984f18b6a515478656d5a0a14ca542c3b7fd54bcd0689b21b898007ae"
      "dacb38bf4536e05887826f90b54077ef02273715a39e70b0043a0208bd094c8d929f3f24"
      "2137624dfcafdd51e653ad69a1ea1c0d4b93f40bad537386a4e579c162c0cc1dc8ca40ae"
      "8f6e1ac287c4652a32caf10697365bcdd2d1996f495ff5017afa915e44360f939d128f5c"
      "784a970a0ba8b0342374c8013aaef7aa9cf8cf36de1690bee5bd40053649ffb9f4308bcf"
      "c5e4e71b58661ff3aec0f582aec74988e2a8b8016a18593b66cc7b17a509cfa9dcd1180f"
      "9f34b9a899240d24c3f1b73061c87bde5ee4bff1cc7d4e198ae683d45c7bcf3b8f78ae32"
      "f55626b6486778ecb28e8fc3c5e17e83ebc3952d7a0fde4c66cbae3a32e9b01c092a7878"
      "a67da91f1510bd7a3d993a009b73e47e0cce3a23b332ea20fc485beae1188c15c3a41ed2"
      "5d47719d1e889a0ccc9eefbdcceb4b7eeac9c20f7d8a541de3a2dafbd9820a7556b0d8db"
      "5f290ddbc4a5cb6e53d316ceeb79bf285abb6a6444e9a0a12e9ba02a6786068da49557c3"
      "3c10a4d5515439b58b0bf030b19d89bfe59374e7f2408f922d4f03058240d9d1467670ac"
      "7d77faea506077164aca85cbeb9727d24b67430c68d524cf49c577475aa16b8db0bbcc66"
      "65a3ab0f76f9bfccd5acd190ddfd4fcc91176cebaa3b11e5ee4f09932361b99096d34504"
      "2169e21bcef3f267432c51ac86859d9d837ed40c0397f665016c16805710e81b86f5d47a"
      "ddcf06c3f955ad34a895f18a006dc1ab3a8bfef4ee38ac95e354862fed2eb47a7e63482b"
      "94a0d268aa8fabd4f4f8123e70d73eecfc83b0076a3bb70486da5acfece2a028a0f9756c"
      "071164e2706b5b61ed49b4fb9aefc0866a22f6052cf7fe51b52c58b9cca771f18ac068ee"
      "7b957eec4e159d37093cc8e38912b004beed4ba7f6a8eb7e87deff7fa027e513eb11b731"
      "5eebc1af06dec70bf5734d2169dcc9e56a8ff304873f64aa1fdbf056bdb527e7f8646f36"
      "947b97a89210740b1852fbcaf4eb971269cbf39dc14ea031115f07f8cb13e8de7a1e1f7f"
      "ed6df22958307e5b949ee6a119269b29b35cd7fe1d80e905db9a4c0745e468a2a7402f07"
      "f1ec78a169b9e6b3ed45d16628b6767d7c2bc60c145624c482bd6cf07b7e5421c8f08125"
      "c0a290c0885d60ab8fbbda3d5bd2ad57bfa7b8e9fcf2035442258a1e5671a6429d0997e9"
      "3d4af222abcc93c3c88a3adb43122e457c5770e6eb1dd18ab867ebf8df2da7a97c369d31"
      "bf02c0842c9f6d4150f1f069c4d56f15e71267a271cfa3decd34d6f55f08c35ef4068845"
      "3dd3bc787b3549d12b81354b6c770c9d0a7798c38848973fac8584a99652fafd7913792e"
      "20486c15716ed84901cec633505d8776c1a23779633713f109d0c752ccc4bc6f0b145d4d"
      "61694fef5dddbd1c268c1bb752c68d1e867f67094d8d48a68bb236763b52b1dab8f71466"
      "16d835378f102878ab0353eb467c3b0b8955b25a03ed808b3e76984d6a70b3e2bd3f061f"
      "3ca0b914f7971e222e6e671db09c292d9ddd5fd8cc3c6cbf3a58679e279a4e024aa91afa"
      "72c6600997547d8c2983ffbf2a1b9f74397db9769f00a27b2e2e842f996a9b9e6734e1fd"
      "d0ae7e5afe229efb099b9c5637d49fb79ae48126360e973a721e262814f561a79b192711"
      "3427d55c1c16076cd2477dda3547cf51dc0527968e2b42802399bc2246b70f6d458559e8"
      "ad1b6fe8719e3742690b4dc744a5971bc904d5b5d3a77cba25aa512e5853c10ff1a2149b"
      "918c2b8cddfc2eca0102f23104fe459dc9b6edff59150cce8da087a2823766c13c20cbc1"
      "9800856eb0f08b1341189a5f2e4325b662d1afee46701ae6d05b7153b10f35a7cb624d59"
      "a490dc9eb7890dcd2b4c04b65c9cb0a1ab988f6f5cc4322dc448c7d63dbf240bcbaeb3a0"
      "6540ecb120027fa5e83063b641b49fcfbb7bd4c60d140874458f59bb4f140dad5a659e2f"
      "3a9e45741f4fd291103bedd1c35f90fbd5984819dce1783a8223c9b16221e4104190a590"
      "167614b50576fce1d9ed11f75ab07b04de64d1edb1f98b8b76711cdc46155c8f19fe2620"
      "1a5eb1c570b15d285a3caab93b54b16fb1a021912f757b7c217cc43ae068fb8729f26a58"
      "dd705f482754d23fa5c9eee2db4abe3e98461baf21a3d0b2a7ca3fb9bdbcaa79c3fa82f6"
      "80218bd56037d8c1a7489c3e0fcd0929dbe8f4f75f55b46ee1928a44ed5d40a52ce95f15"
      "c2d252252c9881155ecfe6b13164ef169de8afee49a6d6806950763acd76bbc059657342"
      "09ffd5af42ce51c532e82bdd9c9d21c56457b5a069565adece2ae11a0ab11083522baf20"
      "96c5b5c2761669e18821122055a4b0a1b7508aa30240af21f86f0dbd5a6992b25b305121"
      "f506cf24521f2fa4dd3a675109b59495937853b88951528c564a040a30b9db585d0c837a"
      "9a671f95483b96847e35759173a7dd157aec996dcb6ae91deb8787e12b393e48b96a5522"
      "dfc00d421000e5f7459efae0dff97f3cbd451d03",
      ToHex(ZCashSerializer::SerializeRawTransaction(tx)));

  EXPECT_EQ(
      ToHex(ZCashSerializer::CalculateTxIdDigest(tx)),
      "0x708a584388071316fc28637726b609daea67911f60195943b26d73260408f7b7");
}

TEST(ZCashSerializerTest, OrchardBundle) {
  ZCashKeyring keyring(
      std::vector<uint8_t>({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}),
      mojom::KeyringId::kZCashMainnet, base::BindRepeating(IsAddressAllowed));

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
      "0x10a9b751104fc2ca413fa45efaab70bcf6eaa3bafe865402a97db549456cd1ec",
      ToHex(tx.orchard_part().digest.value()));

  auto transparent_signature_digest = ZCashSerializer::CalculateSignatureDigest(
      tx, tx.transparent_part().inputs[0]);
  EXPECT_EQ(
      "0x37df59fbf0244263fe9eae69d44b54f7eda9f6cc6cdb1b72b044e478628fc61a",
      ToHex(transparent_signature_digest));
  auto transparent_signature =
      keyring.SignMessage(*key_id, transparent_signature_digest);
  ZCashSerializer::SerializeSignature(tx, tx.transparent_part().inputs[0],
                                      keyring.GetPubkey(*key_id).value(),
                                      transparent_signature.value());
  EXPECT_EQ(
      "0x47304402207b4c3b9b952c74c7a339c22712369835c63304a22421499fcc3bd4dab4d8"
      "96d10220653ca88f6dce51e1c2c12208f1d8871213d388308ce157541c909014d951f535"
      "0121022b19592aabd4f5cff59b4f842ae09155cb0440019ad1fa80086ab35b3acf565b",
      ToHex(tx.transparent_part().inputs[0].script_sig));

  auto shielded_sighash =
      ZCashSerializer::CalculateSignatureDigest(tx, std::nullopt);

  EXPECT_EQ(
      "0xb74780ff85431f696720b96f122a000a252ca02774c8e2c6c65656814087de20",
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
      "17dccdb3858439dbea6fadbeeabf0ac892bc6f2c59cc9344d9f1b781671ae7c9df2fb969"
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
      "6572acdcca3fd1f0708fdb6b156c8e8bfa046a4d3d9b9ac7c8812be6a0bb8ccd9abedd75"
      "202451ca522cff1643fda3e399bf60b2b7393a6f89dd8dca6a1c1ede5965975a62c0088d"
      "3a5e68c0b635535c053509c7f55bf9261ecf9b5c2ca045762df3966fe4c1e97af1c6e3a1"
      "ec3eb3f9b57bfdf44069a9d45105bc00a04f80f9e15f02b34aa42ca95ec8527ea52d52ce"
      "05750a1d67216a3b2f334682f65ce5ea49599bd7e01f98d14c07095635740c3bf644a7d5"
      "eb61c16d28a5bdfdfacb311a29f6b62fe50b71864f70937ba483b96ab0c4f6c566ddd1bb"
      "55536a33a20a8b4f9642f67e8e16662017b947df57320a51cd1141d8e46a40a8c9de24b3"
      "0c0b08957050b693dc18c5acc1482e07204cf113ac974dde5d62044ad29910f5a59256ad"
      "08352c7141f6b65043a86e93a3a25fdba8f6f72b60a9e5276da3140f4e36573ed72aedcb"
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
      "4bc82ae3601b82407069e4dd8a2c4b72909e2810435816ed3747e85302c90381e0de95bd"
      "cba770d8cdc76f49592c2fb524668a1c25641604b1bf523281c8f7c5a80a93d967ced997"
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
      "481766eb4da18047bb78528fc839874e682a5f5da8b81126d424a473f01fbf836b67063a"
      "f731511389f8b15d808ac03c051e139c1485f2f307c4ba18285d929c8374a4ce4615bf7c"
      "bdccb1ae249fc153c3be51ffbddd1c451fc9bf40a9a93149e1d48734681d40309e1171ae"
      "1cb958283d754fca9967dd660560af6298abb134c135b462707d73882a0e242aff9d8b1c"
      "ec16aa279ae4a89b88a58f04854b5c29fe6ddfce9906f8fbdb3454fdcba69b599935523b"
      "8dbffa48e0ebbb6c7e49f8b6cde4991ae5ae8e73ad4e3c88d1beb5087f5d3ecb59599244"
      "10467e588275cd683ac9913e39b6647c1f2724d25b35aa99271f7bde0901c1a645dede05"
      "8ca818daadf0aa1387870bfd725aca1de71621fdd84fded361ea029eeb17aa80e2e178a5"
      "c93e61a8ee39014d87e622f6b33e65801530fd8be52e21432baea08ffeba7c9757c855e0"
      "9ca10e8ff4ee263f8a8988891a1e1427acc5e3bf42360421380d45a358d8ff13487e7881"
      "eaac14b3a01bcf95c024752c408c063484d0afd7cbf0cff6b1627f719a4abb7784409f58"
      "9cb08e6235bc3ebeb07c30275dedb5fa2912f948b372c4a58c51afe63f62eb90e73d2f94"
      "20e6f6901421db31680af1c7cdbbce74c76b9878d4bed60428bd16f4c5bf65a36305ad6e"
      "6a0096597b76ee9b6146e4662ad81539c1e15789be15edbcb10aecc7c3c14dc19c8f89c7"
      "cf38dcb0f89c95819c022f7765de3eb69c28c29d2b2a817d16f518e8f838f1d61dec0f17"
      "aa316df1f1415e0d0ced69a5f9b207742195185a7537d9409eece17be3edd8e2ca60ba30"
      "f69a5f7bb3905dfdb5b549857a94e1d77722506b309d50ceba16940dbc74c1d07b64d211"
      "2dc4db9a2dab8de9bb00c98574d817429454fcd39e03cccc8eb16baf2b75dbf57f30ab59"
      "7810a654ebbdbd938d9e0079759f02f23cca228aace2d62f2f441e139f468b3b460db6b2"
      "18921cf2bb1d29084a6b62f32f4372db53af5fd4a4fc9ed42c9e30f6741dc0ba921af264"
      "e5e22708fd7c7e8cb3333fd64f8296076c3c870c6e5e7d836b6c9ae1b8a10db4060fbb7f"
      "eb4d14d22e658ddf9fa9f4062e4be0c1219cd0a283cdccbd59858498ba9feb2b81b51299"
      "8024cd664200f1a415f67840a9b57711a50e4f70620283be73aa9248c24cc5a6d7bfc760"
      "26536102ae03e5865f7addf2cf50bd8b8c2c692c153cb1e79bb1a678b15b2285afc22cb3"
      "d86fb3150453a65bbb153ebdbb2053afa5bf9e06a7c2fc33a05376ff946c0dc4804c6d10"
      "387d75fef89dd9ae1ebd07a19ca0a7ccaba6fcc15e1e81c6f567af44ea7d056f22bc8c78"
      "577b5f3f8abcf7ed78d65d20086eeaebd55c516c1d0c301e6b5e2d34e5e17c356d0644ff"
      "4d13cb714169bb875fe0e7f1e23dbe269a7db431d009c0cf5278226e506f4339eabb6a57"
      "c9a063536143a06ce4360c478507694e9f5eec8412fefad5afab18268721d37338968162"
      "1865bcc3274d4cb1c23c9c66a7c3550dc3a7ba347bc37317952ce61e10d5a83ed8db8f12"
      "e374ee01a95eae4a5e196408d593031cbc80d200520ddeccb502745f63e58ffb366b6984"
      "87bd8fff04896ee63a14047971db2995bd0fa70cef18d3bcc07ff26ecb93631628c648cb"
      "301128f954a8905bcf630910942222d97fd2c19093bec44a4e14d1ccf20f6d6ef1ef051a"
      "5bbbbd186cc7a0dd543494c3bb2e22910cdf16b8a5b942f588d93ebbeeb460bc68346705"
      "1fd91a5990232fd7870808b2afdca11ecba3f48bd13728ca92d3cb4b246a19135efa0910"
      "59342a2e07bf6877b432f0907fce138978910655348e98738e202baa47ab43de6406add2"
      "4b6158a9be2480bb99002267a3568767c91779a17d6f52ad69935aa5762bb27ae3c7df27"
      "b5a571f0c29c8e6dc7405ecd7d3b5e9e292a0029338e4921fb27e9b48cc7a8d8113a9c28"
      "0e921bb3299c951040a52b8351a36db5639300873a1c453a218fee076b577b75c9f967d8"
      "4b064b412eea444cd886ed00376cd9a9f50a8c2f98813fe90509f3ac32c4ef70f96ff50f"
      "0a5adbe1378911885ea17fae2c086396d2a554e4c6957d4a029af9db9f7877d6b834c600"
      "ded1c9a19e6c020f21343d8a1bd1dbb2cb4b7d521f3c23f683af0a6898b6a201a578386a"
      "a49675defc27a24308926db5d05216f529791e57c516144b20bf64a2a24459374645506a"
      "4708c9f8931e7f2752a02d366a52663568eb60d49750d6b67984b1f46d4c83f2d62b8cd8"
      "3bf9b8e75a11ffce4d677a0d44e52c313ec7daaf2a08f55c36d576a3df0a19ae0660dd77"
      "5b65057d80108231883be98da9fd1c1ef91a8ebfd5767e3e4212a270d7f8b3996ebca6ad"
      "f59a1c3642826a6ea2e6e3e69d45f71cac2a76ab362f638eaf6bb6ae075c67a88aeb0cbb"
      "1cdcbae9647d9077860a1dab2ea297aadc32814317401479c937d9eb5556065a56269cc6"
      "2a659d47f1848f440d72684039049bb2cd3c12a9cfac2c3154b502e1c70fd128c09061fe"
      "ce6d11151f134e918538859f53f288268906b0ec1e38fad3a6b885ea2eb213e38f427708"
      "191191c7542e805640dbecbcb1611e0ab2a97554b1d7fcd95b4bf6d6dbd5ba1692958ba6"
      "1f3b0eaef635d19ab734fc52ec1894a020a35d7b2850ac9e382c627c81bb71b490119cf4"
      "d1cf4a8776ce6e78c1eda66e309ecf1aa0b496980575fbe1129c417ca425b1e1c9dbeef6"
      "588987230e3c6426efb89097404be0b1715dcdcb58397e275927c5b6349a1390ac5caec6"
      "72258abb410f95b9a326d060896578b978099b5c7b052e6e202b379f4a346ef6125450e6"
      "be87aea5f7e3fdb2a827ee66c21a15fc793caff5d955ccf98470fbd9c17c65ebe8d42f94"
      "af2f89621f6002f7a9433145061d6f6340abee484a3db8d3887ce62b5f17e5fd2ec907a2"
      "e4eff25858b56128812c499ff64e6fae4f42a1c7f55e084e4b8e9bbe5523c07f7ae04367"
      "39df0c890d1120aa3d25ecfaf19fd85ac05126838ee6e4566816ce639eab871be07c6688"
      "a815f9eff05a99d66431303b6c7f3124c636f40eb3692ac73b2acaa96769e8d13e1ea492"
      "b65ff2ff019b51bbd390e4f7f9f773374e4784695936cffe9e711530a5141fec7c7c80ca"
      "c6d0a4898437bf1cfaef872058a6ba53df2251b9d2bf28370405d3c7cc19892e3f3ac288"
      "e93d573920270ac6cf1819aeebeeeae31d187abc633ad028746eff97908a5e9b9910d0ff"
      "97a87c1251e31ed452ad8b65956cae8c0e36295a7d29640b8b8f28b354d1bff7a8266f66"
      "2bba6345c51871cd803259d535345ff84e6dc9f06baefb74e0a40cd5b1549007522bf0ef"
      "8424ad749a5c7f049b28e3cbdff699084d9b23a85691c74e8e30c05c29713189e43cce11"
      "680c7dd5eb1fff7c8f9ea3f4e01478c9b8994130d709495b6b3d8a24a60a2e2a22936f9d"
      "7004062dc16cabf340d122d56b97b41bcd9404b3c96e92edb0071ac093a76707f03758d7"
      "4bd705c5c086ec283684adfa5787a5a980ad8dec9b5b039d149099207729d43844719009"
      "6ef4a36d40eb12f3c3ec1f20ab911ef74b159bc9165c81163826bf581a8e8de9be03afc9"
      "b8b83e09c93408c3b15a18e0e235260275d76656b02247a5a45449e4e13dfe156974802e"
      "ba36f1f6023cd0d1e29e4310c0a8a2adc52bf091d150772212a71bfb644c8723a87ddb92"
      "2084eea17b07341e4bb656bbcf2fc43cfeb31f9488a1e2edf6f38732874f4eb8e1876c93"
      "3e9d1c9fd852ed5e502dbbff19e97e131b172d279237c7a9018d21f2fc53255b8a4b6a99"
      "ab3181f9931485ef6613942a48a4f6db61c623c4ef45f0ba12ed92f4eb296e2e923ab3fe"
      "350310b6920b24f301049a4ad148328cd838b8f4eb76fce34fab371fb79ddafd4510fd49"
      "83639343402d4729b391cd48f58e0b8e81aaaea6d8f510a1e2af52bd52145086fe1f1af2"
      "d7f35f92c31c0799f6dbf0e3f9612969f5d0b7269e9919e0372988735cee9e5ea82e9b2d"
      "4a9e7e2c9aac9b866dc819870b3f4aa844909b8dd92b4b71a95ad7c3c4c8418ba20ae8b9"
      "3f6d01cdfe0acdb1b670dfc4e67064eb3038d66233b534df0fab557f1673bd5c39d204b5"
      "234baaa42daf13a5916b5b9cc02d4529171e3432b497a537220c2b01ffb9d2a033d151ee"
      "ea3ba8ac70596577012d8afe34b35accc35787144d6f9fcc3c0e0e81d51bee3a0c36522f"
      "fea5bf35140ca939159a0fb26d4188429a8d0a0bec678ff8a51e2fb23582ad3132566ea0"
      "071e645b35d4306be68392e26f6a2e07a3050638ba8b618ca61dcfbf8078dbe5951bab3a"
      "781c2b5add9ecbf3f16704685bec584bc740c3d25dd2b5030f2c16a4c6150ef08035c71e"
      "7a8794744dad1c871010a129619cbe5a43044e99c6bc82dc5f32f5a9db2cc7f705778dae"
      "f24c1579257f7b2b28caeb3414a1efd324c393aef43982bc6c7e6bf3681126cdc64ce891"
      "c64843067a591959b54fa3a67b836a95471f5860434935fd393daa7ed4706e60074bfa88"
      "bd95089153cdb491ef5ecfe33f31f8523b2a69834661ebd31144a61d7d57f12d90a38895"
      "710084a3627777f06b083f17a1a3082fd942cc4cfeb9ff3a5890d8f57523d8e23d72c5a3"
      "7d8a1fbb5a1f5946d50c14e9da99a4811ed654329e3565d5f8ad1f509faa45ff5301179e"
      "552fdd058afb000d3b2fbbc63737cf7aeaeaa7e5a22acb13fd375bdf65e6088f4b083992"
      "15603ce67aae31ff4e1af879b5ab8ae15a3dad10852e39ab9418a641cf362a974c7f691c"
      "1bf5d676c910afaa876343ff478b0131b4ecf45a592e876a7117962c0ea5827dc4948679"
      "63a0135f1dc55333eb470ca90bdcdfe933ba28f3472fd69694913947ed7e85ebd3bb00fa"
      "683004834280a01ee36d2e2fc7e4abc060260d41bfa74b91397b8310245a74a41b3850aa"
      "52b8fc6e004779b8e55eb7d4c5226ffdb61e2fbd994304f4207ca694eb1c36d80e0a3313"
      "3df39e309bdee0a8b71c3cde50d163c99489722ed83b64af06a7db89f27daa0c524fca1e"
      "364fb41bf92c1573c69863248f583e10dceb5721663631231aceccb08bc7a0d0a7a0366e"
      "fb35e1ec10441f147b39422742eb6d80ecf2262c55d2ada155d38db817ffd2af5f2b793c"
      "321952fe880de9d79970985737e79aeecaa5f7e95f5e8bb931978572fe26e45271518be0"
      "e9f8b753a989d469c78e48568731ed646521335007160fb2c010b567a7c84953f5ab7fe6"
      "e551e9804aa8b6b4b8ca202b7c8960ef6afb4bd9f632b3eca68b1568cd4cbcb2fe31493e"
      "34b706d913eb0d6212e4be27b9748e42613ecab0028456f159342e7c6b64f1a95976250a"
      "daa30b09efd212b00becd7607d28521b4dfe17fb9457aa1239e99c61498d96cf4ae8274d"
      "4b165a3b540a94ceee24b129cfd1d8d1402b9e7d89da9c5cec126cbea6f4fd9c27c9d019"
      "02a8ef17ee04df30427937c97275c82fda21abfb172928135f0ff39e6c0c4c3a90affe7d"
      "a02e378e7c919b8275a06a70cb7fb67fc428a97a82e7303cf5036a947e096b11e312c75b"
      "e91fb79431b0fd8ee6203b179e84948598ad99b90e42a7919ca8a8cdde0024c035905c0b"
      "66a8456dae121f09ba6b7e12c3dc76b7bb1ea6ba8a370468a528b6d66e22ee0f485fe76b"
      "686b9dcdda12306cc89bd1cdd644a6aac17c7a8b9134b3b88f39b7884d319eaf87c8fda5"
      "eb89a3a1a537cc0498c3eb963d58c4bea20431f8f0c6810f01f8c4f20aa3940255871500"
      "7aae5909c7a79638af1304c11305d7823b1680b0b6f6e6733ef3a5c4f02552842cc02326"
      "155f48cac2bc8b7c403092b77835107642df6fee36c8762a2a89060519809112b77314cb"
      "dd96d2e49a3994a3c74004cab1444e4a08e7f034f5ab492d39b5bda6ed3b17c5ea4beb2c"
      "1f37de25e63a2f3037ef01cc0c8eac9a8dab4d414a35c050e16ee73e794391827607bbcf"
      "abe1e2af3346cf3291e29a7eb513c0270b8ad4f1f4b582df893671977804d014b444494b"
      "d365f5e6e86e2c4566d49fb0371e02013a223781150ad61bc1086c35e72f0efd1664026c"
      "fded50c8f2ec24e7d6bb1028e25202c6ad2c2487502c8f87d84e952ecc26d1316bf6efbf"
      "0c02eadf036b37e83712a9145acb572c790c82bd055b815fed79d3ec7d7040217edf2b7b"
      "7ec9a7537888954d8d919317483146fc0fbebbb9a41279677b00da3f97fa8b5eb5a6120a"
      "861a4df82ebc37ae88349147ae3d836393909eaff06b2de2c0e5c2f74823d4ee5958c03f"
      "34ca379db4224b3b15360369eeea2dd00175363e09656a1f148ec170b4acd4118d6d3a5e"
      "bf1892689cf5f21bb029d9d1292f68af5ccbc5350a4150876b00cb1f7760aff5c5011918"
      "e0ea1f1d6e2b4c414220034917b3794114d12e65e1285fcb3b6e60adb71edd17f6a0379c"
      "6ab61cc60f409b4714075246245df14f1d1a1fe9f68c0981182130b4a7257f0b5f39d663"
      "1ab9332810258141159f23ebe859bd8c2b3e3edbca3fb3d938d6755c63e9c8c47ca8b01a"
      "1b973a382116eeecff55f86b17e524ac490ac29b654f701e95d2f169cb77c0c3472707ef"
      "7d651197e97c000d57c4b193f439f7d51e57a09923f3e826830893cd10203aac90f68b39"
      "d6eb2fa11e40e7bbbe1c4bfcdbce75f230fab973b402bc409613e5eb76ca49e47998defe"
      "89e598a3272a80b248d26d486a5869acf5993e7923860497dc584054112c62ca9bcc7c2a"
      "9d03e967285620b6a4f47c46a600027627a00031e346418e1e261dccb9a6d9dbc31575f5"
      "a6a8084730869a31ab93d2b0d596555622d34f5ef3eae103063f7f111e2f12c6297ba59a"
      "bc2f945c57f76241d5b3075741ca4d1fb61527f3b0d8867e8e0d024abcdb0e76dd2ae7a6"
      "505c7c7dbb9149b8f9c231f27b54f8830408bcef761b3d5cdcab3d0ad5ecc00528254681"
      "29f45623883aa8004bc7d559de9d538dc417876744e619dd22166ad28c05d41cf0ba7399"
      "685cea0a73deeb21d1ebe8d95727b013901c7c88e83286f089e1100571c5f87daa1c80c0"
      "0bfbac48cdb97484ce14aeef75457f2553d6cbba98127b90b9687839872b5e974ff1c407"
      "5eb1da3a77210155a1591db9e67478b0e7cbbac77faa98f310d84e9158784dc02d9b8b5a"
      "360864d7a41dcb4d9bc079b521e1c20d7304ab9fa109badd977d7b6ee2dc72dfca0fc15d"
      "1a645b73c72eb1566712f870017e6c9331ab715f1ebf0bbc47b620727a2a753baf298e44"
      "da02f98f0dfbc70207c0614079bb3c5326df6f2bf430bcd23011b9e816e9a58597e1ccca"
      "2120054f3245efeba3cbda361900a4e84cf3eb9c500638a02a69e0136e656cb687e8f0f7"
      "542d12317d153a4096e8a92d1cd1d7b5993296236a2e8e4da931446cf506d7ccacd072a7"
      "38d3749f44837f6838451aa13523456a2f55800a26e35661a32ba8cc59a249a0352804b9"
      "768c6e7d5740907ad7120323fee2dc48871f713ff9dfd2f11ac574d29e504f3b21f90557"
      "36cb5521f33de5470966db3209997a8a9aab49f555afa7389beeeda53f6d32bb8c7aa9be"
      "292e19b9c648fedae31063e88fe1d48e0c594f6e3d9f9208d348eaaf67f11da80219f9c8"
      "ac37fc6deba689d8ed01e80952d620f837f22d7383b0aa0cefc55b644f10463a18e0be75"
      "d07f91f9b799a7c23504d02ba57fabdf7d36bb56b5f1673bf508a874788f124e0076250b"
      "8e8b694e44cec723c4aca61e1ff3283a017b9297bb270c66f15dfa168eff2cf831c963f6"
      "2d00713d598a8663abe60b340ba887ca9c14e68ae57105c6f15478880fe11e460ca407bc"
      "327ac950b84c6e36454909e7a219988b2bb12b1e603b351a47eb9b97327f75af9f45e7e2"
      "39cda33ab4ae64e8613b86ee385ea1ea71c48486078eb98a7e22ff3ea3c1c0262c09f959"
      "1518f4b7b337dd070ee8b2fa77bc7bbeefb211948775a65518049f0ddf04851dd55b9a0f"
      "da15dc3ecf4044402c2dfd31705a9be7a1e0f91201babcc6f4560914f9ef403a200f266b"
      "7bae5fb358483f8bd7655bb91dc55fdf8fdd25647f1a8ea09d0304582b0fc62c52c5e538"
      "01b25c6a3f2483daf26dbde00088aaaa74b4fbdbfa647b018e3c5cb60cd39ae5728534d9"
      "0fffe3ebb4f599dc6374673d6b4233abe7deed00b03769b0d2db50c439b69bf629754832"
      "c376f298cca45350652c66faa3d7c46a2202006a56e99ad333f54cfc7aa8ac97259e2188"
      "6692bbadeb26d96109f828c7101d8d4eb81d1475b103d4921f0e018fb7e97dbb93302492"
      "79c3ee701d4e8872752779c2d7c1a4d69eef68ca9675de39dccd30aa961520b78d9f87e5"
      "610bee993d9be098854f9e519dc4cd0d6d21df5f346982629f8065acf9fea200d208d8e1"
      "379879e7ae57a9913406481cca655ab92cd9148cba6588e4f505d653076dc924789e1545"
      "cf93f00beb8baff741627a29753aa10398b79b2de04c7fac7012d539280cf4a9270f5ff5"
      "deb8a59bdcee9fe470a3721831fbe45f6fc33a0f624279e4ae94540ec3c9b24a5798b1cc"
      "cdac03d56d982a77e99afc501a2a2b19e77c943ea63b9353df46934441a0f7573d71f900"
      "426db52510105f1396111d2996d05e37ee305af1b94974f6ca598de47ede840e564575af"
      "a8c1f983728a946bdf55f4ca6221854f0f232037a88afb544e636f41f4dc16b0b8219b5f"
      "b2145093211214c32f3258fad0b2fa29a0a99a2b86410f79dd1afd3e919ef41d049a138b"
      "b1b5dadd0a99b5b2fdc6102de50ab1944f59df2c497f2d3b6a4bd582befd3e6893934bee"
      "20bec200db40a5a789b08b00428a4f012b126e97c63801c25936767055c4da3d6ebc690f"
      "e51fdb66fc521c25a400298e184bb488190dd6e294567860222020f3b00b5962d1cdb7e5"
      "de985c7b59288eacef0c00936f86ebf1e7976db206e07eeb39b1377ea6ef1a87da8d7206"
      "ed736205c38b457f1b6a9cdbdf1a4998b8b9b40caa90ed99cbb8b7612a4d78f7b61f049b"
      "411c832d012f4252811db98ef547908a0982008d64d92360d7c4d0ead15affc8bcdfc279"
      "3d1a800a18c98e128e469c19c6006c10e12ed47e2555edb405a4cdac23d336302f7cdf6a"
      "91dcd9f507872b349d17da364af37e5d495c4e19c0b68a2ee3c9dc3cefd3e6dccb242eb0"
      "56995c1a32206680fb8fea36398f06160355e59d6b33d6708bec100b7e865250c505fa84"
      "cf15c5373257817dddf35e943044623141e9aba4f69c6de03b3297ad13231e18f43d9831"
      "cf4953d8056c533c7c5aac06d307d165647aef1f330aa819ebd46cea4f317d71a881eeb4"
      "422bb99e486782873b77bbe7c8c393bd7b8f0e9baa60fde55910e46a15608fb837b4eaba"
      "aa66c591e2206cb13fe64271930132d99a188d7ac718c244e82ed65505efeb09e49eb2e1"
      "f534e486dced14a9f1c9ef8c9173a174978ef8924bcd48885933725001d6b20d76add7ec"
      "0f1f48099228d40b0f184c9657206f94f96929f102015cf0a9a158b0f79a6ef77e79b60c"
      "3743252ea423416df405560832e81672cf290adf6db5f622d69ad5a6c5ec468dfb33044b"
      "4b56b5e5a51e7b4e30b9d4a478f44833cd5d4d2a6e03c350ab0fae5a07298a050f34ba6a"
      "5606ab261f6e2b001246a5b2ab8f5209df22e6eb3cdc41b63ab1f7ce4b8726e8922e");

  EXPECT_EQ(
      "0x050000800a27a726b4d0d6c20000000068be190001146b9d49dd8c7835f43a37dca078"
      "7e3ec9f6605223d5ba7ae0ab9025b73bc03f010000006a47304402207b4c3b9b952c74c7"
      "a339c22712369835c63304a22421499fcc3bd4dab4d896d10220653ca88f6dce51e1c2c1"
      "2208f1d8871213d388308ce157541c909014d951f5350121022b19592aabd4f5cff59b4f"
      "842ae09155cb0440019ad1fa80086ab35b3acf565bffffffff000000024330112c31d0cb"
      "b0db4f77cefc9036ed215ea86ba2aee9a5de08953a8216282f5136e62a35a7a471c0a68f"
      "7a2b09915993244010af0ce69e863a0cdc68042b103dd62d315080c0bec76397d7c6ad36"
      "5d84a6354d07049e484d0240410c589389eeb33ccbb564cc383b01383d4bac03648d6f15"
      "5fc5d3d280f0f85a3687f17b0e90a1dc6793503417e53794980e4b8b5e1a18e454da912f"
      "a17818988a7b0b1006fedd657b795cc584843cb90f9cdf67912f2017dccdb3858439dbea"
      "6fadbeeabf0ac892bc6f2c59cc9344d9f1b781671ae7c9df2fb969fce6c8215fa7feae4b"
      "e87a0b5bb08266bead9d4ee0f80433cd17b713643a5f02ce53392cb015418fc4ff336ea3"
      "2d7a749448802b7f828d51d51d78e707d268959c856d9239a6eff5090d1813581a19c24d"
      "f421a03eea8bd7e9f8a9cf938c35fe65cf213a1a4b6c727e9401cc519f3ef3f2c9360791"
      "49bc2af3083d8a963811c761138e44383c08c34de0d483f62879c4cb80c5cb84124da36a"
      "be113d68eadd017c5c268d6fbdd9c661753fe5375c8043fa3e9f9c94a8e1035bdd2e8eaa"
      "3a814d0a62657bc5fe689568b5a41ab718342b4790520e1bceb54b987d2590354b54fe69"
      "5ca24c6585bb591cdc2ac395f06a07194812aeceaac0cdbc4178b68be11e30352956b250"
      "378495bfaa4b326d264db71b83b28ef164300dee9a17f187617596c071f63d9383aeecec"
      "6f28e1080013e674b2412d15dd667e2532339caed06f4db6da25e7fcc326a1a58469d94d"
      "f082166dbb24a1117898bf7c497466690d13949cd1db05a516622cb2d746469d04e716ac"
      "ebb04f1c519f00d26dac539a71c2e4c9f1efa0cec80ba0cf37a29cd375e0fa70146becfc"
      "66ef4cace008fc8a22ae77aa718ce5a149256df6407a24d33e5eb1e9d51b2bd2ba4d71b7"
      "1f3bb54538e984f719bbb2daebec2a54476a4104daa47d591fd9db9820f68d48cf52dee3"
      "f8772fdd25de4ae436ea1cf125296c30e91262b407c80188224c356572acdcca3fd1f070"
      "8fdb6b156c8e8bfa046a4d3d9b9ac7c8812be6a0bb8ccd9abedd75202451ca522cff1643"
      "fda3e399bf60b2b7393a6f89dd8dca6a1c1ede5965975a62c0088d3a5e68c0b635535c05"
      "3509c7f55bf9261ecf9b5c2ca045762df3966fe4c1e97af1c6e3a1ec3eb3f9b57bfdf440"
      "69a9d45105bc00a04f80f9e15f02b34aa42ca95ec8527ea52d52ce05750a1d67216a3b2f"
      "334682f65ce5ea49599bd7e01f98d14c07095635740c3bf644a7d5eb61c16d28a5bdfdfa"
      "cb311a29f6b62fe50b71864f70937ba483b96ab0c4f6c566ddd1bb55536a33a20a8b4f96"
      "42f67e8e16662017b947df57320a51cd1141d8e46a40a8c9de24b30c0b08957050b693dc"
      "18c5acc1482e07204cf113ac974dde5d62044ad29910f5a59256ad08352c7141f6b65043"
      "a86e93a3a25fdba8f6f72b60a9e5276da3140f4e36573ed72aedcb4ae68ac15a57667193"
      "3c1a102d172031fb0d586b1ffb70828ff5a3ff02b38d67153388bfcf5b6d02bb476225fc"
      "50786083d09fad59b0b97da176b7b4787277d4f90e090b8e6f370b38eada32da32734553"
      "d64b4681c21af7ffe2bcb8d914e43cf24a953a0f94edd975409bf5927de38e324ea2e52b"
      "f4590b0391dff3ded4354d0bfe2ae3acb4a9302124c73718ca2d2a30667be950fad78bb9"
      "fe49f8ffa40afb40a2ab26ecd17bc9ed16dc0e848550825fd093c6f1c4eafcb1c7c98596"
      "02f7fede5fbe5becf84f28bf74abc880e56b77ed7a7242448da180f07ffe854a7fa321a2"
      "722cdb9c1c46b93186ad15001b32c0a7de4a02a1526b334d97c2b36f6d0fe83d3f219837"
      "c2adc186d6ef1a3f1e0a50d6ca2fc1c2c2a2ea95373d6a2fd9906f49296f3e8bc141a8c6"
      "bbc904722c2541756622d7284e45697c6f0f1a141b786e80fe882391d7c5919bb98e2fa1"
      "719da21c9e4d04a0c30888b0b608d46a054c32b75bee0396efdeb310e8c5211d41728ab7"
      "96a44cdb543468e0d655cf64afd62e71dd4688eb7e2e196025f1ef0c2cbc99cc08075cb3"
      "6b99cca1600edd3f6451eedff9c3adeb7b843deb83a5582e51ad32881b8ca6064986a41d"
      "26d91d0c48adef928d766c7f58bd9a31da0096c7bf18a9f8c307f14bc82ae3601b824070"
      "69e4dd8a2c4b72909e2810435816ed3747e85302c90381e0de95bdcba770d8cdc76f4959"
      "2c2fb524668a1c25641604b1bf523281c8f7c5a80a93d967ced9978830b45c2dbf64d9ca"
      "8cdc6498082ae003d50eafee3c930d40d253f4bfe65ec69e7260d3b99dbed28acab956ba"
      "79ff0107d586c878760caa9ac9036079feffffffffffae2935f1dfd8a24aed7c70df7de3"
      "a668eb7a49b1319880dde2bbd9031ae5d82ffd601c6d8bb1d603470a10040bb503334989"
      "33da7cac03eb16937cef9d35d9371046b2d13387711416e9c51d066f32fdfae7203369a1"
      "449da2700c37bfdb099d24a2be7f8abd0833762284617c9150b81e04d4051e0aaccc7100"
      "179b997b8fa7bfdd305d4183f78f8385336c189426bf0ff42bb5c766a93d2ebb4f3fe898"
      "35da45c106c9dc861332d6f3b6705239303cd9b60210a9a266532934511322f6ba8eac22"
      "8a2df4ddddd778bad54694afc2f7d4159a3363361c4b677d723a7fb56a6f0f2914e7710b"
      "0ba8b8d1c209db43bb4911677f1bc1f7fa3faf5d4498753761c08b5129fe69223b94ebb0"
      "0548ab19310d28c02a3930c68b0cdfa548106476c7d98b46b56e441701fa115d58c76a52"
      "3ee2711d13b2b75e45ac466ed3ca501b690d89a92047c72cfe3dbbddd9ba784458ac8520"
      "e369993b31535f3585b7e0679a5dabbcac553e904db5b8f5884679ff1fd49ff4cb9f57ee"
      "3fba42e5dfce4de71f26f11c94da728f9d159ce803c145fd1fa3070d6c6d4318d303d050"
      "334374a84eaf66ba2f9672239e32ad301a542d3d569c94d23a9a9b424069c4a66261fee5"
      "ff3c66282f6ed9e89898f5ca0c0b26ff37f21633bf35cacfc848fc192a7be3641eeadd1d"
      "34641912dd53e4400391bc5341ff004f14dbd35fe96960f86062dfc49092178ab0b05a87"
      "867c4656825f5c818da35d74628ec85d08d398644573789a1018c1a21a4acd1012a10086"
      "b00f96d6083abfd79337c0a3c44dae81cd14c2a4cad615668e0407bd305f77179d26b17b"
      "f314c0437848334eaab3ea92f27ed59ef928b7d03b5bf1b910033eb416772a28e3639998"
      "39175d75bb67ad350df97ad2d20a262e2e8c72e0ee08371d8382645a4237e2327d6c1ed6"
      "02a04e0f5658776884234c32b15a98a7cb4310cacbdfddf80cbde428705cd193a7bd2fca"
      "e9b8541f06557348226a5d18f9668b4bd86ef9171305ad9649ad36481766eb4da18047bb"
      "78528fc839874e682a5f5da8b81126d424a473f01fbf836b67063af731511389f8b15d80"
      "8ac03c051e139c1485f2f307c4ba18285d929c8374a4ce4615bf7cbdccb1ae249fc153c3"
      "be51ffbddd1c451fc9bf40a9a93149e1d48734681d40309e1171ae1cb958283d754fca99"
      "67dd660560af6298abb134c135b462707d73882a0e242aff9d8b1cec16aa279ae4a89b88"
      "a58f04854b5c29fe6ddfce9906f8fbdb3454fdcba69b599935523b8dbffa48e0ebbb6c7e"
      "49f8b6cde4991ae5ae8e73ad4e3c88d1beb5087f5d3ecb5959924410467e588275cd683a"
      "c9913e39b6647c1f2724d25b35aa99271f7bde0901c1a645dede058ca818daadf0aa1387"
      "870bfd725aca1de71621fdd84fded361ea029eeb17aa80e2e178a5c93e61a8ee39014d87"
      "e622f6b33e65801530fd8be52e21432baea08ffeba7c9757c855e09ca10e8ff4ee263f8a"
      "8988891a1e1427acc5e3bf42360421380d45a358d8ff13487e7881eaac14b3a01bcf95c0"
      "24752c408c063484d0afd7cbf0cff6b1627f719a4abb7784409f589cb08e6235bc3ebeb0"
      "7c30275dedb5fa2912f948b372c4a58c51afe63f62eb90e73d2f9420e6f6901421db3168"
      "0af1c7cdbbce74c76b9878d4bed60428bd16f4c5bf65a36305ad6e6a0096597b76ee9b61"
      "46e4662ad81539c1e15789be15edbcb10aecc7c3c14dc19c8f89c7cf38dcb0f89c95819c"
      "022f7765de3eb69c28c29d2b2a817d16f518e8f838f1d61dec0f17aa316df1f1415e0d0c"
      "ed69a5f9b207742195185a7537d9409eece17be3edd8e2ca60ba30f69a5f7bb3905dfdb5"
      "b549857a94e1d77722506b309d50ceba16940dbc74c1d07b64d2112dc4db9a2dab8de9bb"
      "00c98574d817429454fcd39e03cccc8eb16baf2b75dbf57f30ab597810a654ebbdbd938d"
      "9e0079759f02f23cca228aace2d62f2f441e139f468b3b460db6b218921cf2bb1d29084a"
      "6b62f32f4372db53af5fd4a4fc9ed42c9e30f6741dc0ba921af264e5e22708fd7c7e8cb3"
      "333fd64f8296076c3c870c6e5e7d836b6c9ae1b8a10db4060fbb7feb4d14d22e658ddf9f"
      "a9f4062e4be0c1219cd0a283cdccbd59858498ba9feb2b81b512998024cd664200f1a415"
      "f67840a9b57711a50e4f70620283be73aa9248c24cc5a6d7bfc76026536102ae03e5865f"
      "7addf2cf50bd8b8c2c692c153cb1e79bb1a678b15b2285afc22cb3d86fb3150453a65bbb"
      "153ebdbb2053afa5bf9e06a7c2fc33a05376ff946c0dc4804c6d10387d75fef89dd9ae1e"
      "bd07a19ca0a7ccaba6fcc15e1e81c6f567af44ea7d056f22bc8c78577b5f3f8abcf7ed78"
      "d65d20086eeaebd55c516c1d0c301e6b5e2d34e5e17c356d0644ff4d13cb714169bb875f"
      "e0e7f1e23dbe269a7db431d009c0cf5278226e506f4339eabb6a57c9a063536143a06ce4"
      "360c478507694e9f5eec8412fefad5afab18268721d373389681621865bcc3274d4cb1c2"
      "3c9c66a7c3550dc3a7ba347bc37317952ce61e10d5a83ed8db8f12e374ee01a95eae4a5e"
      "196408d593031cbc80d200520ddeccb502745f63e58ffb366b698487bd8fff04896ee63a"
      "14047971db2995bd0fa70cef18d3bcc07ff26ecb93631628c648cb301128f954a8905bcf"
      "630910942222d97fd2c19093bec44a4e14d1ccf20f6d6ef1ef051a5bbbbd186cc7a0dd54"
      "3494c3bb2e22910cdf16b8a5b942f588d93ebbeeb460bc683467051fd91a5990232fd787"
      "0808b2afdca11ecba3f48bd13728ca92d3cb4b246a19135efa091059342a2e07bf6877b4"
      "32f0907fce138978910655348e98738e202baa47ab43de6406add24b6158a9be2480bb99"
      "002267a3568767c91779a17d6f52ad69935aa5762bb27ae3c7df27b5a571f0c29c8e6dc7"
      "405ecd7d3b5e9e292a0029338e4921fb27e9b48cc7a8d8113a9c280e921bb3299c951040"
      "a52b8351a36db5639300873a1c453a218fee076b577b75c9f967d84b064b412eea444cd8"
      "86ed00376cd9a9f50a8c2f98813fe90509f3ac32c4ef70f96ff50f0a5adbe1378911885e"
      "a17fae2c086396d2a554e4c6957d4a029af9db9f7877d6b834c600ded1c9a19e6c020f21"
      "343d8a1bd1dbb2cb4b7d521f3c23f683af0a6898b6a201a578386aa49675defc27a24308"
      "926db5d05216f529791e57c516144b20bf64a2a24459374645506a4708c9f8931e7f2752"
      "a02d366a52663568eb60d49750d6b67984b1f46d4c83f2d62b8cd83bf9b8e75a11ffce4d"
      "677a0d44e52c313ec7daaf2a08f55c36d576a3df0a19ae0660dd775b65057d8010823188"
      "3be98da9fd1c1ef91a8ebfd5767e3e4212a270d7f8b3996ebca6adf59a1c3642826a6ea2"
      "e6e3e69d45f71cac2a76ab362f638eaf6bb6ae075c67a88aeb0cbb1cdcbae9647d907786"
      "0a1dab2ea297aadc32814317401479c937d9eb5556065a56269cc62a659d47f1848f440d"
      "72684039049bb2cd3c12a9cfac2c3154b502e1c70fd128c09061fece6d11151f134e9185"
      "38859f53f288268906b0ec1e38fad3a6b885ea2eb213e38f427708191191c7542e805640"
      "dbecbcb1611e0ab2a97554b1d7fcd95b4bf6d6dbd5ba1692958ba61f3b0eaef635d19ab7"
      "34fc52ec1894a020a35d7b2850ac9e382c627c81bb71b490119cf4d1cf4a8776ce6e78c1"
      "eda66e309ecf1aa0b496980575fbe1129c417ca425b1e1c9dbeef6588987230e3c6426ef"
      "b89097404be0b1715dcdcb58397e275927c5b6349a1390ac5caec672258abb410f95b9a3"
      "26d060896578b978099b5c7b052e6e202b379f4a346ef6125450e6be87aea5f7e3fdb2a8"
      "27ee66c21a15fc793caff5d955ccf98470fbd9c17c65ebe8d42f94af2f89621f6002f7a9"
      "433145061d6f6340abee484a3db8d3887ce62b5f17e5fd2ec907a2e4eff25858b5612881"
      "2c499ff64e6fae4f42a1c7f55e084e4b8e9bbe5523c07f7ae0436739df0c890d1120aa3d"
      "25ecfaf19fd85ac05126838ee6e4566816ce639eab871be07c6688a815f9eff05a99d664"
      "31303b6c7f3124c636f40eb3692ac73b2acaa96769e8d13e1ea492b65ff2ff019b51bbd3"
      "90e4f7f9f773374e4784695936cffe9e711530a5141fec7c7c80cac6d0a4898437bf1cfa"
      "ef872058a6ba53df2251b9d2bf28370405d3c7cc19892e3f3ac288e93d573920270ac6cf"
      "1819aeebeeeae31d187abc633ad028746eff97908a5e9b9910d0ff97a87c1251e31ed452"
      "ad8b65956cae8c0e36295a7d29640b8b8f28b354d1bff7a8266f662bba6345c51871cd80"
      "3259d535345ff84e6dc9f06baefb74e0a40cd5b1549007522bf0ef8424ad749a5c7f049b"
      "28e3cbdff699084d9b23a85691c74e8e30c05c29713189e43cce11680c7dd5eb1fff7c8f"
      "9ea3f4e01478c9b8994130d709495b6b3d8a24a60a2e2a22936f9d7004062dc16cabf340"
      "d122d56b97b41bcd9404b3c96e92edb0071ac093a76707f03758d74bd705c5c086ec2836"
      "84adfa5787a5a980ad8dec9b5b039d149099207729d438447190096ef4a36d40eb12f3c3"
      "ec1f20ab911ef74b159bc9165c81163826bf581a8e8de9be03afc9b8b83e09c93408c3b1"
      "5a18e0e235260275d76656b02247a5a45449e4e13dfe156974802eba36f1f6023cd0d1e2"
      "9e4310c0a8a2adc52bf091d150772212a71bfb644c8723a87ddb922084eea17b07341e4b"
      "b656bbcf2fc43cfeb31f9488a1e2edf6f38732874f4eb8e1876c933e9d1c9fd852ed5e50"
      "2dbbff19e97e131b172d279237c7a9018d21f2fc53255b8a4b6a99ab3181f9931485ef66"
      "13942a48a4f6db61c623c4ef45f0ba12ed92f4eb296e2e923ab3fe350310b6920b24f301"
      "049a4ad148328cd838b8f4eb76fce34fab371fb79ddafd4510fd4983639343402d4729b3"
      "91cd48f58e0b8e81aaaea6d8f510a1e2af52bd52145086fe1f1af2d7f35f92c31c0799f6"
      "dbf0e3f9612969f5d0b7269e9919e0372988735cee9e5ea82e9b2d4a9e7e2c9aac9b866d"
      "c819870b3f4aa844909b8dd92b4b71a95ad7c3c4c8418ba20ae8b93f6d01cdfe0acdb1b6"
      "70dfc4e67064eb3038d66233b534df0fab557f1673bd5c39d204b5234baaa42daf13a591"
      "6b5b9cc02d4529171e3432b497a537220c2b01ffb9d2a033d151eeea3ba8ac7059657701"
      "2d8afe34b35accc35787144d6f9fcc3c0e0e81d51bee3a0c36522ffea5bf35140ca93915"
      "9a0fb26d4188429a8d0a0bec678ff8a51e2fb23582ad3132566ea0071e645b35d4306be6"
      "8392e26f6a2e07a3050638ba8b618ca61dcfbf8078dbe5951bab3a781c2b5add9ecbf3f1"
      "6704685bec584bc740c3d25dd2b5030f2c16a4c6150ef08035c71e7a8794744dad1c8710"
      "10a129619cbe5a43044e99c6bc82dc5f32f5a9db2cc7f705778daef24c1579257f7b2b28"
      "caeb3414a1efd324c393aef43982bc6c7e6bf3681126cdc64ce891c64843067a591959b5"
      "4fa3a67b836a95471f5860434935fd393daa7ed4706e60074bfa88bd95089153cdb491ef"
      "5ecfe33f31f8523b2a69834661ebd31144a61d7d57f12d90a38895710084a3627777f06b"
      "083f17a1a3082fd942cc4cfeb9ff3a5890d8f57523d8e23d72c5a37d8a1fbb5a1f5946d5"
      "0c14e9da99a4811ed654329e3565d5f8ad1f509faa45ff5301179e552fdd058afb000d3b"
      "2fbbc63737cf7aeaeaa7e5a22acb13fd375bdf65e6088f4b08399215603ce67aae31ff4e"
      "1af879b5ab8ae15a3dad10852e39ab9418a641cf362a974c7f691c1bf5d676c910afaa87"
      "6343ff478b0131b4ecf45a592e876a7117962c0ea5827dc494867963a0135f1dc55333eb"
      "470ca90bdcdfe933ba28f3472fd69694913947ed7e85ebd3bb00fa683004834280a01ee3"
      "6d2e2fc7e4abc060260d41bfa74b91397b8310245a74a41b3850aa52b8fc6e004779b8e5"
      "5eb7d4c5226ffdb61e2fbd994304f4207ca694eb1c36d80e0a33133df39e309bdee0a8b7"
      "1c3cde50d163c99489722ed83b64af06a7db89f27daa0c524fca1e364fb41bf92c1573c6"
      "9863248f583e10dceb5721663631231aceccb08bc7a0d0a7a0366efb35e1ec10441f147b"
      "39422742eb6d80ecf2262c55d2ada155d38db817ffd2af5f2b793c321952fe880de9d799"
      "70985737e79aeecaa5f7e95f5e8bb931978572fe26e45271518be0e9f8b753a989d469c7"
      "8e48568731ed646521335007160fb2c010b567a7c84953f5ab7fe6e551e9804aa8b6b4b8"
      "ca202b7c8960ef6afb4bd9f632b3eca68b1568cd4cbcb2fe31493e34b706d913eb0d6212"
      "e4be27b9748e42613ecab0028456f159342e7c6b64f1a95976250adaa30b09efd212b00b"
      "ecd7607d28521b4dfe17fb9457aa1239e99c61498d96cf4ae8274d4b165a3b540a94ceee"
      "24b129cfd1d8d1402b9e7d89da9c5cec126cbea6f4fd9c27c9d01902a8ef17ee04df3042"
      "7937c97275c82fda21abfb172928135f0ff39e6c0c4c3a90affe7da02e378e7c919b8275"
      "a06a70cb7fb67fc428a97a82e7303cf5036a947e096b11e312c75be91fb79431b0fd8ee6"
      "203b179e84948598ad99b90e42a7919ca8a8cdde0024c035905c0b66a8456dae121f09ba"
      "6b7e12c3dc76b7bb1ea6ba8a370468a528b6d66e22ee0f485fe76b686b9dcdda12306cc8"
      "9bd1cdd644a6aac17c7a8b9134b3b88f39b7884d319eaf87c8fda5eb89a3a1a537cc0498"
      "c3eb963d58c4bea20431f8f0c6810f01f8c4f20aa39402558715007aae5909c7a79638af"
      "1304c11305d7823b1680b0b6f6e6733ef3a5c4f02552842cc02326155f48cac2bc8b7c40"
      "3092b77835107642df6fee36c8762a2a89060519809112b77314cbdd96d2e49a3994a3c7"
      "4004cab1444e4a08e7f034f5ab492d39b5bda6ed3b17c5ea4beb2c1f37de25e63a2f3037"
      "ef01cc0c8eac9a8dab4d414a35c050e16ee73e794391827607bbcfabe1e2af3346cf3291"
      "e29a7eb513c0270b8ad4f1f4b582df893671977804d014b444494bd365f5e6e86e2c4566"
      "d49fb0371e02013a223781150ad61bc1086c35e72f0efd1664026cfded50c8f2ec24e7d6"
      "bb1028e25202c6ad2c2487502c8f87d84e952ecc26d1316bf6efbf0c02eadf036b37e837"
      "12a9145acb572c790c82bd055b815fed79d3ec7d7040217edf2b7b7ec9a7537888954d8d"
      "919317483146fc0fbebbb9a41279677b00da3f97fa8b5eb5a6120a861a4df82ebc37ae88"
      "349147ae3d836393909eaff06b2de2c0e5c2f74823d4ee5958c03f34ca379db4224b3b15"
      "360369eeea2dd00175363e09656a1f148ec170b4acd4118d6d3a5ebf1892689cf5f21bb0"
      "29d9d1292f68af5ccbc5350a4150876b00cb1f7760aff5c5011918e0ea1f1d6e2b4c4142"
      "20034917b3794114d12e65e1285fcb3b6e60adb71edd17f6a0379c6ab61cc60f409b4714"
      "075246245df14f1d1a1fe9f68c0981182130b4a7257f0b5f39d6631ab933281025814115"
      "9f23ebe859bd8c2b3e3edbca3fb3d938d6755c63e9c8c47ca8b01a1b973a382116eeecff"
      "55f86b17e524ac490ac29b654f701e95d2f169cb77c0c3472707ef7d651197e97c000d57"
      "c4b193f439f7d51e57a09923f3e826830893cd10203aac90f68b39d6eb2fa11e40e7bbbe"
      "1c4bfcdbce75f230fab973b402bc409613e5eb76ca49e47998defe89e598a3272a80b248"
      "d26d486a5869acf5993e7923860497dc584054112c62ca9bcc7c2a9d03e967285620b6a4"
      "f47c46a600027627a00031e346418e1e261dccb9a6d9dbc31575f5a6a8084730869a31ab"
      "93d2b0d596555622d34f5ef3eae103063f7f111e2f12c6297ba59abc2f945c57f76241d5"
      "b3075741ca4d1fb61527f3b0d8867e8e0d024abcdb0e76dd2ae7a6505c7c7dbb9149b8f9"
      "c231f27b54f8830408bcef761b3d5cdcab3d0ad5ecc0052825468129f45623883aa8004b"
      "c7d559de9d538dc417876744e619dd22166ad28c05d41cf0ba7399685cea0a73deeb21d1"
      "ebe8d95727b013901c7c88e83286f089e1100571c5f87daa1c80c00bfbac48cdb97484ce"
      "14aeef75457f2553d6cbba98127b90b9687839872b5e974ff1c4075eb1da3a77210155a1"
      "591db9e67478b0e7cbbac77faa98f310d84e9158784dc02d9b8b5a360864d7a41dcb4d9b"
      "c079b521e1c20d7304ab9fa109badd977d7b6ee2dc72dfca0fc15d1a645b73c72eb15667"
      "12f870017e6c9331ab715f1ebf0bbc47b620727a2a753baf298e44da02f98f0dfbc70207"
      "c0614079bb3c5326df6f2bf430bcd23011b9e816e9a58597e1ccca2120054f3245efeba3"
      "cbda361900a4e84cf3eb9c500638a02a69e0136e656cb687e8f0f7542d12317d153a4096"
      "e8a92d1cd1d7b5993296236a2e8e4da931446cf506d7ccacd072a738d3749f44837f6838"
      "451aa13523456a2f55800a26e35661a32ba8cc59a249a0352804b9768c6e7d5740907ad7"
      "120323fee2dc48871f713ff9dfd2f11ac574d29e504f3b21f9055736cb5521f33de54709"
      "66db3209997a8a9aab49f555afa7389beeeda53f6d32bb8c7aa9be292e19b9c648fedae3"
      "1063e88fe1d48e0c594f6e3d9f9208d348eaaf67f11da80219f9c8ac37fc6deba689d8ed"
      "01e80952d620f837f22d7383b0aa0cefc55b644f10463a18e0be75d07f91f9b799a7c235"
      "04d02ba57fabdf7d36bb56b5f1673bf508a874788f124e0076250b8e8b694e44cec723c4"
      "aca61e1ff3283a017b9297bb270c66f15dfa168eff2cf831c963f62d00713d598a8663ab"
      "e60b340ba887ca9c14e68ae57105c6f15478880fe11e460ca407bc327ac950b84c6e3645"
      "4909e7a219988b2bb12b1e603b351a47eb9b97327f75af9f45e7e239cda33ab4ae64e861"
      "3b86ee385ea1ea71c48486078eb98a7e22ff3ea3c1c0262c09f9591518f4b7b337dd070e"
      "e8b2fa77bc7bbeefb211948775a65518049f0ddf04851dd55b9a0fda15dc3ecf4044402c"
      "2dfd31705a9be7a1e0f91201babcc6f4560914f9ef403a200f266b7bae5fb358483f8bd7"
      "655bb91dc55fdf8fdd25647f1a8ea09d0304582b0fc62c52c5e53801b25c6a3f2483daf2"
      "6dbde00088aaaa74b4fbdbfa647b018e3c5cb60cd39ae5728534d90fffe3ebb4f599dc63"
      "74673d6b4233abe7deed00b03769b0d2db50c439b69bf629754832c376f298cca4535065"
      "2c66faa3d7c46a2202006a56e99ad333f54cfc7aa8ac97259e21886692bbadeb26d96109"
      "f828c7101d8d4eb81d1475b103d4921f0e018fb7e97dbb9330249279c3ee701d4e887275"
      "2779c2d7c1a4d69eef68ca9675de39dccd30aa961520b78d9f87e5610bee993d9be09885"
      "4f9e519dc4cd0d6d21df5f346982629f8065acf9fea200d208d8e1379879e7ae57a99134"
      "06481cca655ab92cd9148cba6588e4f505d653076dc924789e1545cf93f00beb8baff741"
      "627a29753aa10398b79b2de04c7fac7012d539280cf4a9270f5ff5deb8a59bdcee9fe470"
      "a3721831fbe45f6fc33a0f624279e4ae94540ec3c9b24a5798b1cccdac03d56d982a77e9"
      "9afc501a2a2b19e77c943ea63b9353df46934441a0f7573d71f900426db52510105f1396"
      "111d2996d05e37ee305af1b94974f6ca598de47ede840e564575afa8c1f983728a946bdf"
      "55f4ca6221854f0f232037a88afb544e636f41f4dc16b0b8219b5fb2145093211214c32f"
      "3258fad0b2fa29a0a99a2b86410f79dd1afd3e919ef41d049a138bb1b5dadd0a99b5b2fd"
      "c6102de50ab1944f59df2c497f2d3b6a4bd582befd3e6893934bee20bec200db40a5a789"
      "b08b00428a4f012b126e97c63801c25936767055c4da3d6ebc690fe51fdb66fc521c25a4"
      "00298e184bb488190dd6e294567860222020f3b00b5962d1cdb7e5de985c7b59288eacef"
      "0c00936f86ebf1e7976db206e07eeb39b1377ea6ef1a87da8d7206ed736205c38b457f1b"
      "6a9cdbdf1a4998b8b9b40caa90ed99cbb8b7612a4d78f7b61f049b411c832d012f425281"
      "1db98ef547908a0982008d64d92360d7c4d0ead15affc8bcdfc2793d1a800a18c98e128e"
      "469c19c6006c10e12ed47e2555edb405a4cdac23d336302f7cdf6a91dcd9f507872b349d"
      "17da364af37e5d495c4e19c0b68a2ee3c9dc3cefd3e6dccb242eb056995c1a32206680fb"
      "8fea36398f06160355e59d6b33d6708bec100b7e865250c505fa84cf15c5373257817ddd"
      "f35e943044623141e9aba4f69c6de03b3297ad13231e18f43d9831cf4953d8056c533c7c"
      "5aac06d307d165647aef1f330aa819ebd46cea4f317d71a881eeb4422bb99e486782873b"
      "77bbe7c8c393bd7b8f0e9baa60fde55910e46a15608fb837b4eabaaa66c591e2206cb13f"
      "e64271930132d99a188d7ac718c244e82ed65505efeb09e49eb2e1f534e486dced14a9f1"
      "c9ef8c9173a174978ef8924bcd48885933725001d6b20d76add7ec0f1f48099228d40b0f"
      "184c9657206f94f96929f102015cf0a9a158b0f79a6ef77e79b60c3743252ea423416df4"
      "05560832e81672cf290adf6db5f622d69ad5a6c5ec468dfb33044b4b56b5e5a51e7b4e30"
      "b9d4a478f44833cd5d4d2a6e03c350ab0fae5a07298a050f34ba6a5606ab261f6e2b0012"
      "46a5b2ab8f5209df22e6eb3cdc41b63ab1f7ce4b8726e8922e",
      ToHex(ZCashSerializer::SerializeRawTransaction(tx)));
}

}  // namespace brave_wallet
