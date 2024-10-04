/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"

#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {
constexpr char kTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
constexpr char kTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
constexpr char kAddress1[] = "tb1qya3rarek59486w345v45tv6nra4fy2xxgky26x";
constexpr char kAddress2[] = "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm";

}  // namespace

TEST(BitcoinSerializer, SerializeInputForSign) {
  BitcoinTransaction tx;

  BitcoinTransaction::TxInput input1;
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  input1.witness = {4, 5, 6};
  tx.AddInput(input1);

  BitcoinTransaction::TxInput input2;
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  input2.script_sig = {1, 2};
  input2.witness = {4, 5};
  tx.AddInput(std::move(input2));

  BitcoinTransaction::TxOutput output1;
  output1.address = kAddress1;
  output1.script_pubkey =
      BitcoinSerializer::AddressToScriptPubkey(kAddress1, true);
  output1.amount = 5;
  tx.AddOutput(std::move(output1));

  BitcoinTransaction::TxOutput output2;
  output2.address = kAddress2;
  output2.script_pubkey =
      BitcoinSerializer::AddressToScriptPubkey(kAddress2, true);
  output2.amount = 50;
  tx.AddOutput(std::move(output2));

  tx.set_locktime(777);

  EXPECT_EQ(base::HexEncode(*BitcoinSerializer::SerializeInputForSign(tx, 0)),
            "25395E842E3005AC64B1B23CEA639C1899A3C2D18EBF58CC47679EBE3EC810F9");
  EXPECT_EQ(base::HexEncode(*BitcoinSerializer::SerializeInputForSign(tx, 1)),
            "FBD8650BA68214C9659928A7E16A6B4148D895755BC5036B328532CAFC4267FB");

  // P2PKH addresses are not suppported.
  input1.utxo_address = "1N4Qbzg6LSXUXyXu2MDuGfzxwMA7do8AyL";
  tx.ClearInputs();
  tx.AddInput(input1);
  EXPECT_FALSE(BitcoinSerializer::SerializeInputForSign(tx, 0));

  // P2SH addresses are not suppported.
  input1.utxo_address = "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy";
  tx.ClearInputs();
  tx.AddInput(input1);
  EXPECT_FALSE(BitcoinSerializer::SerializeInputForSign(tx, 0));

  // P2TR addresses are not suppported.
  input1.utxo_address =
      "bc1peu5hzzyj8cnqm05le6ag7uwry0ysmtf3v4uuxv3v8hqhvsatca8ss2vuwx";
  tx.ClearInputs();
  tx.AddInput(input1);
  EXPECT_FALSE(BitcoinSerializer::SerializeInputForSign(tx, 0));
}

TEST(BitcoinSerializer, SerializeWitness) {
  std::vector<uint8_t> signature = {0, 1, 2, 3};
  std::vector<uint8_t> pubkey = {0xaa, 0xbb, 0xcc, 0xdd};
  EXPECT_EQ(
      base::HexEncode(BitcoinSerializer::SerializeWitness(signature, pubkey)),
      "02040001020304AABBCCDD");
}

TEST(BitcoinSerializer, SerializeSignedTransaction) {
  std::vector<uint8_t> signature = {0, 1, 2, 3};
  std::vector<uint8_t> pubkey = {0xaa, 0xbb, 0xcc, 0xdd};

  BitcoinTransaction tx;

  BitcoinTransaction::TxInput input1;
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.witness = BitcoinSerializer::SerializeWitness(signature, pubkey);
  tx.AddInput(std::move(input1));

  BitcoinTransaction::TxInput input2;
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  input2.witness = BitcoinSerializer::SerializeWitness(signature, pubkey);
  tx.AddInput(std::move(input2));

  BitcoinTransaction::TxOutput output1;
  output1.address = kAddress1;
  output1.script_pubkey =
      BitcoinSerializer::AddressToScriptPubkey(kAddress1, true);
  output1.amount = 5;
  tx.AddOutput(std::move(output1));

  BitcoinTransaction::TxOutput output2;
  output2.address = kAddress2;
  output2.script_pubkey =
      BitcoinSerializer::AddressToScriptPubkey(kAddress2, true);
  output2.amount = 50;
  tx.AddOutput(std::move(output2));

  tx.set_locktime(777);

  EXPECT_EQ(
      base::HexEncode(BitcoinSerializer::SerializeSignedTransaction(tx)),
      "02000000000102C5E29F841382F02A49BEAFAC756D14A211EC9089AD50E153767625B750"
      "8F38AA7B00000000FDFFFFFFBEC2C52B2448A8733648E967D2B4559D0F1AA4BBBB93E53E"
      "9F516A12FB9C1CBD0700000000FDFFFFFF02050000000000000016001427623E8F36A16A"
      "7D3A35A32B45B3531F6A9228C63200000000000000160014674F8F912B5A9305F5D3A348"
      "F9B069D9101173E902040001020304AABBCCDD02040001020304AABBCCDD09030000");

  EXPECT_EQ(BitcoinSerializer::CalcTransactionWeight(tx, false), 640u);
  EXPECT_EQ(BitcoinSerializer::CalcTransactionVBytes(tx, false), 160u);
}

TEST(BitcoinSerializer, AddressToScriptPubkey_BitcoinCoreTestVectors) {
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(
      base::PathService::CheckedGet(base::DIR_GEN_TEST_DATA_ROOT)
          .Append(
              FILE_PATH_LITERAL("brave/wallet-test-data/key_io_valid.json")),
      &file_contents));
  auto test_items = base::test::ParseJsonList(file_contents);
  uint32_t total_tests = test_items.size();
  uint32_t skipped_tests = 0;

  std::vector<std::string> not_supported_addresses = {
      // witness v2, too short
      "bc1z2rksukkjr8",
      // witness v3, too short
      "tb1rgv5m6uvdk3kc7qsuz0c79v88ycr5w4wa",
      // witness v2, too short
      "bc1zmjtqxkzs89",
      // witness v3, too short
      "tb1r0ecpfxg2udhtc556gqrpwwhk4sw3f0kc",
      // witness v3
      "tb1rx9n9g37az8mu236e5jpxdt0m67y4fuq8rhs0ss3djnm0kscfrwvq0ntlyg",
  };

  for (auto& test_item : test_items) {
    ASSERT_TRUE(test_item.is_list());
    const auto& address = test_item.GetList()[0].GetString();
    const auto& expected_script =
        base::ToUpperASCII(test_item.GetList()[1].GetString());
    const auto& options = test_item.GetList()[2].GetDict();
    if (options.FindBool("isPrivkey").value()) {
      skipped_tests++;
      continue;
    }
    if (*options.FindString("chain") != "main" &&
        *options.FindString("chain") != "test") {
      skipped_tests++;
      continue;
    }

    if (base::Contains(not_supported_addresses, address)) {
      skipped_tests++;
      continue;
    }
    bool testnet = *options.FindString("chain") == "test";
    auto actual = base::HexEncode(
        BitcoinSerializer::AddressToScriptPubkey(address, testnet));
    EXPECT_EQ(expected_script, actual) << address;
  }
  EXPECT_EQ(70u, total_tests);
  EXPECT_EQ(46u, skipped_tests);
}

TEST(BitcoinSerializer, CalcOutputVBytesInTransaction) {
  {
    // P2WPKH
    BitcoinTransaction::TxOutput output;
    output.address = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4", false);
    output.amount = 5;
    EXPECT_EQ(BitcoinSerializer::CalcOutputVBytesInTransaction(output), 31u);
  }
  {
    // P2WSH
    BitcoinTransaction::TxOutput output;
    output.address =
        "bc1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qccfmv3";
    output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        "bc1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qccfmv3",
        false);
    output.amount = 5;
    EXPECT_EQ(BitcoinSerializer::CalcOutputVBytesInTransaction(output), 43u);
  }
  {
    // P2PKH
    BitcoinTransaction::TxOutput output;
    output.address = "19Sp9dLinHy3dKo2Xxj53ouuZWAoVGGhg8";
    output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        "19Sp9dLinHy3dKo2Xxj53ouuZWAoVGGhg8", false);
    output.amount = 5;
    EXPECT_EQ(BitcoinSerializer::CalcOutputVBytesInTransaction(output), 34u);
  }
  {
    // P2SH
    BitcoinTransaction::TxOutput output;
    output.address = "34jnjFM4SbaB7Q8aMtNDG849RQ1gUYgpgo";
    output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        "34jnjFM4SbaB7Q8aMtNDG849RQ1gUYgpgo", false);
    output.amount = 5;
    EXPECT_EQ(BitcoinSerializer::CalcOutputVBytesInTransaction(output), 32u);
  }
  {
    // P2TR
    BitcoinTransaction::TxOutput output;
    output.address =
        "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0";
    output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0",
        false);
    output.amount = 5;
    EXPECT_EQ(BitcoinSerializer::CalcOutputVBytesInTransaction(output), 43u);
  }
}

TEST(BitcoinSerializer, CalcInputVBytesInTransaction) {
  BitcoinTransaction::TxInput input;
  input.utxo_address = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
  EXPECT_EQ(BitcoinSerializer::CalcInputVBytesInTransaction(input), 68u);
}

TEST(BitcoinSerializer, CalcTransactionWeight) {
  // Tested by SerializeSignedTransaction.
  SUCCEED();
}

TEST(BitcoinSerializer, CalcTransactionVBytes) {
  // Tested by SerializeSignedTransaction.
  SUCCEED();
}

}  // namespace brave_wallet
