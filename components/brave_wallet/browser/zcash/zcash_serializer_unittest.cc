/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(ZCashSerializerTest, HashPrevouts) {
  ZCashTransaction zcash_transaciton;

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                   244, 58,  55,  220, 160, 120, 126, 62,
                                   201, 246, 96,  82,  35,  213, 186, 122,
                                   224, 171, 144, 37,  183, 59,  192, 63};
    tx_input.utxo_outpoint.index = 3224808575;
    zcash_transaciton.inputs().push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {193, 161, 45,  18,  123, 87,  200, 19,
                                   137, 118, 231, 145, 1,   59,  1,   95,
                                   6,   166, 36,  245, 33,  182, 238, 4,
                                   236, 152, 8,   147, 199, 229, 224, 26};
    tx_input.utxo_outpoint.index = 1493393971;
    zcash_transaciton.inputs().push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {208, 145, 48,  246, 53,  17,  218, 84,
                                   131, 45,  233, 19,  107, 57,  244, 89,
                                   159, 90,  165, 223, 187, 69,  218, 96,
                                   205, 206, 171, 126, 239, 222, 137, 190};
    tx_input.utxo_outpoint.index = 3237475171;
    zcash_transaciton.inputs().push_back(std::move(tx_input));
  }

  ASSERT_EQ(
      "0x7db761d908021c98a19c43f75c6486275eaca3c11f9dc6cbaf66d3050c23b515",
      ToHex(ZCashSerializer::HashPrevouts(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashOutputs) {
  ZCashTransaction zcash_transaciton;

  {
    ZCashTransaction::TxOutput tx_output;
    base::HexStringToBytes("630063ac", &tx_output.script_pubkey);
    tx_output.amount = 1264123119664452;
    zcash_transaciton.outputs().push_back(std::move(tx_output));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    base::HexStringToBytes("636a5351520065ac65", &tx_output.script_pubkey);
    tx_output.amount = 810835337737746;
    zcash_transaciton.outputs().push_back(std::move(tx_output));
  }

  ASSERT_EQ(
      "0x0dc9291fc891c10bdecedde449fa319cfa3f45cf7779423c2272c013d7fe0080",
      ToHex(ZCashSerializer::HashOutputs(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashSequences) {
  ZCashTransaction zcash_transaciton;

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 1290119100;
    zcash_transaciton.inputs().push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 3797894359;
    zcash_transaciton.inputs().push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 4015866081;
    zcash_transaciton.inputs().push_back(std::move(tx_input));
  }

  ASSERT_EQ(
      "0x17cae6cde4962f6eb86b350eb5a80d5576a958b4bd3438689e94ee387eb80f8e",
      ToHex(ZCashSerializer::HashSequences(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashHeader) {
  ZCashTransaction zcash_transaciton;
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
    tx_input.script_pub_key = ZCashAddressToScriptPubkey(
        "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi", false);

    tx.inputs().push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
    tx_output.amount = 100000;
    tx_output.script_pubkey = ZCashAddressToScriptPubkey(
        "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi", false);

    tx.outputs().push_back(std::move(tx_output));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi";
    tx_output.amount = 649000;
    tx_output.script_pubkey = ZCashAddressToScriptPubkey(
        "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi", false);

    tx.outputs().push_back(std::move(tx_output));
  }

  auto tx_id = ZCashSerializer::CalculateTxIdDigest(tx);

  ASSERT_EQ(
      ToHex(tx_id),
      "0x360d056309669faf0d7937f41581418be5e46b04e2cea0a7b14261d7bff1d825");
}

}  // namespace brave_wallet
