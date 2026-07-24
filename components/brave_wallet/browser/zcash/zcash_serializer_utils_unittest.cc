/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer_utils.h"

#include <array>
#include <optional>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

ZCashTransaction::TxInput MakeInput(
    const std::array<uint8_t, 32>& txid,
    uint32_t index,
    uint32_t n_sequence,
    uint64_t utxo_value,
    const std::vector<uint8_t>& script_pub_key) {
  ZCashTransaction::TxInput input;
  input.utxo_outpoint.txid = txid;
  input.utxo_outpoint.index = index;
  input.n_sequence = n_sequence;
  input.utxo_value = utxo_value;
  input.script_pub_key = script_pub_key;
  return input;
}

ZCashTransaction MakeCombinedTransparentTx() {
  ZCashTransaction tx;
  tx.set_consensus_brach_id(0xc2d6d0b4);

  tx.transparent_part().inputs.push_back(MakeInput(
      {20,  107, 157, 73,  221, 140, 120, 53, 244, 58, 55,
       220, 160, 120, 126, 62,  201, 246, 96, 82,  35, 213,
       186, 122, 224, 171, 144, 37,  183, 59, 192, 63},
      3224808575u, 1290119100u, 1848924248978091u, {0xac, 0x00, 0x00}));
  tx.transparent_part().inputs.push_back(
      MakeInput({193, 161, 45,  18,  123, 87,  200, 19,  137, 118, 231,
                 145, 1,   59,  1,   95,  6,   166, 36,  245, 33,  182,
                 238, 4,   236, 152, 8,   147, 199, 229, 224, 26},
                1493393971u, 3797894359u, 555666777u, {0x51}));
  tx.transparent_part().inputs.push_back(
      MakeInput({208, 145, 48,  246, 53,  17,  218, 84,  131, 45,  233,
                 19,  107, 57,  244, 89,  159, 90,  165, 223, 187, 69,
                 218, 96,  205, 206, 171, 126, 239, 222, 137, 190},
                3237475171u, 4015866081u, 555u, {0x52}));

  {
    ZCashTransaction::TxOutput output;
    base::HexStringToBytes("630063ac", &output.script_pubkey);
    output.amount = 1264123119664452u;
    tx.transparent_part().outputs.push_back(std::move(output));
  }
  {
    ZCashTransaction::TxOutput output;
    base::HexStringToBytes("636a5351520065ac65", &output.script_pubkey);
    output.amount = 810835337737746u;
    tx.transparent_part().outputs.push_back(std::move(output));
  }
  return tx;
}

}  // namespace

TEST(ZCashSerializerUtilsTest, HashPrevouts) {
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

  EXPECT_EQ(
      "0x7db761d908021c98a19c43f75c6486275eaca3c11f9dc6cbaf66d3050c23b515",
      ToHex(ZCashSerializerUtils::HashPrevouts(zcash_transaciton)));
}

TEST(ZCashSerializerUtilsTest, HashOutputs) {
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

  EXPECT_EQ(
      "0x0dc9291fc891c10bdecedde449fa319cfa3f45cf7779423c2272c013d7fe0080",
      ToHex(ZCashSerializerUtils::HashOutputs(zcash_transaciton)));
}

TEST(ZCashSerializerUtilsTest, HashSequences) {
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

  EXPECT_EQ(
      "0x17cae6cde4962f6eb86b350eb5a80d5576a958b4bd3438689e94ee387eb80f8e",
      ToHex(ZCashSerializerUtils::HashSequences(zcash_transaciton)));
}

TEST(ZCashSerializerUtilsTest, HashTxIn) {
  ZCashTransaction::TxInput tx_input;
  tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                 244, 58,  55,  220, 160, 120, 126, 62,
                                 201, 246, 96,  82,  35,  213, 186, 122,
                                 224, 171, 144, 37,  183, 59,  192, 63};
  tx_input.utxo_outpoint.index = 3224808575;
  tx_input.utxo_value = 1848924248978091;
  tx_input.n_sequence = 1290119100;
  base::HexStringToBytes("ac0000", &tx_input.script_pub_key);

  EXPECT_EQ(
      "0xb39969f0fba708491e480d80d4d675a1f1552cc7d479d7942f75fa31ad9c6ad6",
      ToHex(ZCashSerializerUtils::HashTxIn(tx_input)));
  EXPECT_EQ(
      "0x3eaca6f70479f3ed3db11a001707ef9d8f0f661cd45342473203c86ba1ff8975",
      ToHex(ZCashSerializerUtils::HashTxIn(std::nullopt)));
}

TEST(ZCashSerializerUtilsTest, HashAmounts) {
  ZCashTransaction tx = MakeCombinedTransparentTx();
  EXPECT_EQ(
      "0xcc08b3f2a60ea3615d7165866447bc67a7c17cc2e0aeea122e9a989535f4e724",
      ToHex(ZCashSerializerUtils::HashAmounts(tx)));
  EXPECT_EQ(
      "0xcbdc27bd0946a43495971d2887d6dd1eb4bfde3807b0f74e195e9dc17d4eb3f0",
      ToHex(ZCashSerializerUtils::HashAmounts(ZCashTransaction())));
}

TEST(ZCashSerializerUtilsTest, HashScriptPubKeys) {
  ZCashTransaction tx = MakeCombinedTransparentTx();
  EXPECT_EQ(
      "0x20f5b26f60fdd2d7dbce4a73e33672257a3b5fccd18a6054d0a8c8899b0a16f0",
      ToHex(ZCashSerializerUtils::HashScriptPubKeys(tx)));
  EXPECT_EQ(
      "0xf1c4ad734fe0613f1078b0598d9baa9a9663de48f390393cf842aa37fe095b98",
      ToHex(ZCashSerializerUtils::HashScriptPubKeys(ZCashTransaction())));
}

TEST(ZCashSerializerUtilsTest, Blake2b256) {
  EXPECT_EQ(
      "0x7d856ffddf82f7ab67ff68714af0b7f601c10b92f4dbeb73fa0416efebe3e04b",
      ToHex(ZCashSerializerUtils::Blake2b256(
          base::byte_span_from_cstring("hello"),
          base::byte_span_from_cstring("ZTxIdTranspaHash"))));
  EXPECT_EQ(
      "0xc33f2e95705faab35f8d533fa61e95c3b7aaba0776b874a9f74fc12784376a59",
      ToHex(ZCashSerializerUtils::Blake2b256(
          {}, base::byte_span_from_cstring("ZTxIdTranspaHash"))));
}

TEST(ZCashSerializerUtilsTest, GetHashPersonalizer) {
  ZCashTransaction tx;
  tx.set_consensus_brach_id(0xc2d6d0b4);
  EXPECT_EQ("0x5a636173685478486173685fb4d0d6c2",
            ToHex(ZCashSerializerUtils::GetHashPersonalizer(tx)));
}

TEST(ZCashSerializerUtilsTest, PushOutpoint) {
  ZCashTransaction::Outpoint outpoint;
  outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53, 244, 58, 55,
                   220, 160, 120, 126, 62,  201, 246, 96, 82,  35, 213,
                   186, 122, 224, 171, 144, 37,  183, 59, 192, 63};
  outpoint.index = 3224808575u;

  BtcLikeSerializerStream stream;
  ZCashSerializerUtils::PushOutpoint(outpoint, stream);
  EXPECT_EQ(
      "0x146b9d49dd8c7835f43a37dca0787e3ec9f6605223d5ba7ae0ab9025b73bc03f7fac"
      "36c0",
      ToHex(stream.data()));
}

TEST(ZCashSerializerUtilsTest, PushOutput) {
  ZCashTransaction::TxOutput output;
  base::HexStringToBytes("630063ac", &output.script_pubkey);
  output.amount = 1264123119664452u;

  BtcLikeSerializerStream stream;
  ZCashSerializerUtils::PushOutput(output, stream);
  EXPECT_EQ("0x44fd7f99b67d040004630063ac", ToHex(stream.data()));
}

TEST(ZCashSerializerUtilsTest, SerializeTransparentInputs) {
  ZCashTransaction tx = MakeCombinedTransparentTx();
  BtcLikeSerializerStream stream;
  ZCashSerializerUtils::SerializeTransparentInputs(tx, stream);
  EXPECT_EQ(
      "0x03146b9d49dd8c7835f43a37dca0787e3ec9f6605223d5ba7ae0ab9025b73bc03f7f"
      "ac36c000bca7e54cc1a12d127b57c8138976e791013b015f06a624f521b6ee04ec980"
      "893c7e5e01a3362035900d7445fe2d09130f63511da54832de9136b39f4599f5aa5d"
      "fbb45da60cdceab7eefde89be63f3f7c000e1405def",
      ToHex(stream.data()));

  BtcLikeSerializerStream empty_stream;
  ZCashSerializerUtils::SerializeTransparentInputs(ZCashTransaction(),
                                                   empty_stream);
  EXPECT_EQ("0x00", ToHex(empty_stream.data()));
}

TEST(ZCashSerializerUtilsTest, SerializeTransparentOutputs) {
  ZCashTransaction tx = MakeCombinedTransparentTx();
  BtcLikeSerializerStream stream;
  ZCashSerializerUtils::SerializeTransparentOutputs(tx, stream);
  EXPECT_EQ(
      "0x0244fd7f99b67d040004630063ac12f6465073e1020009636a5351520065ac65",
      ToHex(stream.data()));

  BtcLikeSerializerStream empty_stream;
  ZCashSerializerUtils::SerializeTransparentOutputs(ZCashTransaction(),
                                                    empty_stream);
  EXPECT_EQ("0x00", ToHex(empty_stream.data()));
}

TEST(ZCashSerializerUtilsTest, HashTransparentTxId) {
  ZCashTransaction tx = MakeCombinedTransparentTx();
  EXPECT_EQ(
      "0x82a71ae22ccbd3ef500a8fe86d6c226b3c046ef69904df4b77d3f64fc95f1c2b",
      ToHex(ZCashSerializerUtils::HashTransparentTxId(tx)));

  // Empty transparent part hashes the empty payload.
  EXPECT_EQ(
      "0xc33f2e95705faab35f8d533fa61e95c3b7aaba0776b874a9f74fc12784376a59",
      ToHex(ZCashSerializerUtils::HashTransparentTxId(ZCashTransaction())));
}

TEST(ZCashSerializerUtilsTest, HashTransparentSignature) {
  ZCashTransaction tx = MakeCombinedTransparentTx();
  EXPECT_EQ(
      "0xf9457d192a845c818edddcb823f780ab572a7d5552c434cfd1e7fca4f1518c81",
      ToHex(ZCashSerializerUtils::HashTransparentSignature(
          tx, tx.transparent_part().inputs[0])));

  // Outputs-only path (no transparent inputs).
  ZCashTransaction outputs_only;
  outputs_only.transparent_part().outputs =
      std::move(tx.transparent_part().outputs);
  EXPECT_EQ(
      "0xa34af52383cdaa6ea6c0e55b6d0d78e68df15e4f023eea2c925e613dc6f1ad3e",
      ToHex(ZCashSerializerUtils::HashTransparentSignature(outputs_only,
                                                           std::nullopt)));

  // Empty transparent part hashes the empty payload.
  EXPECT_EQ(
      "0xc33f2e95705faab35f8d533fa61e95c3b7aaba0776b874a9f74fc12784376a59",
      ToHex(ZCashSerializerUtils::HashTransparentSignature(ZCashTransaction(),
                                                           std::nullopt)));
}

TEST(ZCashSerializerUtilsTest, CalculateSignatureDigest) {
  ZCashTransaction tx = MakeCombinedTransparentTx();
  tx.set_locktime(1);
  tx.set_expiry_height(10000);

  EXPECT_EQ(ZCashV5Serializer::CalculateSignatureDigest(
                tx, tx.transparent_part().inputs[0]),
            ZCashSerializerUtils::CalculateSignatureDigest(
                tx, tx.transparent_part().inputs[0]));
  EXPECT_EQ(ZCashV5Serializer::CalculateSignatureDigest(tx, std::nullopt),
            ZCashSerializerUtils::CalculateSignatureDigest(tx, std::nullopt));
}

}  // namespace brave_wallet
