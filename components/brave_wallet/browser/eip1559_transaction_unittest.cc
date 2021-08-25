/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/hd_key.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(Eip1559TransactionUnitTest, GetMessageToSign) {
  std::vector<uint8_t> data;
  EXPECT_TRUE(base::HexStringToBytes("010200", &data));
  Eip1559Transaction tx =
      *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
          mojom::TxData::New("0x00", "0x00", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", data),
          "0x04", "0x0", "0x0"));
  ASSERT_EQ(tx.type(), 2);
  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item;
  item.address.fill(0x01);

  Eip2930Transaction::AccessedStorageKey storage_key_1;
  storage_key_1.fill(0x01);
  item.storage_keys.push_back(storage_key_1);

  access_list->push_back(item);

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.GetMessageToSign())),
            "fa81814f7dd57bad435657a05eabdba2815f41e3f15ddd6139027e7db56b0dea");
}

TEST(Eip1559TransactionUnitTest, GetSignedTransaction) {
  const struct {
    const char* nonce;
    const char* value;
    const char* gas_limit;
    const char* max_priority_fee_per_gas;
    const char* max_fee_per_gas;
    const char* signed_tx;
  } cases[] = {
      {"0x333", "0x2933BC9", "0x8AE0", "0x1284D", "0x1D97C",
       "0x02f86e048203338301284d8301d97c828ae094000000000000000000000000000"
       "000000000aaaa8402933bc980c080a00f924cb68412c8f1cfd74d9b581c71eeaf94ff"
       "f6abdde3e5b02ca6b2931dcf47a07dd1c50027c3e31f8b565e25ce68a5072110f61fce5"
       "eee81b195dd51273c2f83"},
      {"0x161", "0x3B08B33", "0x7F51", "0x97C2", "0x21467",
       "0x02f86d048201618297c283021467827f519400000000000000000000000000000"
       "0000000aaaa8403b08b3380c080a08caf712f72489da6f1a634b651b4b1c7d9be7d1e"
       "8d05ea76c1eccee3bdfb86a5a06aecc106f588ce51e112f5e9ea7aba3e089dc75117188"
       "21d0e0cd52f52af4e45"},
      {"0x3D9", "0x1F06571", "0x10BBD", "0x10349", "0x213A1",
       "0x02f86f048203d983010349830213a183010bbd940000000000000000000000000"
       "00000000000aaaa8401f0657180c001a08c03a86e85789ee9a1b42fa0a86d316fca26"
       "2694f8c198df11f194678c2c2d35a028f8e7de02b35014a17b6d28ff8c7e7be6860e726"
       "5ac162fb721f1aeae75643c"},
      {"0x26F", "0x14A5987", "0xE17D", "0x1219C", "0x13D15",
       "0x02f86e0482026f8301219c83013d1582e17d94000000000000000000000000000"
       "000000000aaaa84014a598780c001a0b87c4c8c505d2d692ac77ba466547e79dd60fe"
       "7ecd303d520bf6e8c7085e3182a06dc7d00f5e68c3f3ebe8ae35a90d46051afde620ac1"
       "2e43cae9560a29b13e7fb"},
      {"0x3CC", "0x5A2EC37", "0xFEE6", "0xA72E", "0x1942A",
       "0x02f86d048203cc82a72e8301942a82fee69400000000000000000000000000000"
       "0000000aaaa8405a2ec3780c001a006cf07af78c187db104496c58d679f37fcd2d579"
       "0970cecf9a59fe4a5321b375a039f3faafc71479d283a5b1e66a86b19c4bdc516655d89"
       "dbe57d9747747c01dfe"},
      {"0x24C", "0x5EC1B9F", "0x919A", "0x15752", "0x1FCE1",
       "0x02f86e0482024c830157528301fce182919a94000000000000000000000000000"
       "000000000aaaa8405ec1b9f80c080a03e2f59ac9ca852034c2c1da35a742ca19fdd91"
       "0aa5d2ed49ab8ad27a2fcb2b10a03ac1c29c26723c58f91400fb6dfb5f5b837467b1c37"
       "7541b47dae474dddbe469"},
      {"0x384", "0x1CFE6D1", "0x12915", "0x220A", "0x1B841",
       "0x02f86e0482038482220a8301b8418301291594000000000000000000000000000"
       "000000000aaaa8401cfe6d180c001a0f7ffc5bca2512860f8236360159bf303dcfab715"
       "46b6a0032df0306f3739d0c4a05d38fe2c4edebdc1edc157034f780c53a0e5ae089e572"
       "20745bd48bcb10cdf87"},
      {"0x2C5", "0x62D8DB", "0x6EAF", "0x150EC", "0x171AC",
       "0x02f86d048202c5830150ec830171ac826eaf94000000000000000000000000000"
       "000000000aaaa8362d8db80c001a0a61a5710512f346c9996377f7b564ccb64c73a5fdb"
       "615499adb1250498f3e01aa002d10429572cecfaa911a58bbe05f2b26e4c3aee3402202"
       "153a93692849add11"},
      {"0x3AB", "0x2A76B9", "0xAFF7", "0xB0A0", "0x16600",
       "0x02f86c048203ab82b0a08301660082aff79400000000000000000000000000000"
       "0000000aaaa832a76b980c001a0191f0f6667a20cefc0b454e344cc01108aafbdc4e4e5"
       "ed88fdd1b5d108495b31a020879042b0f8d3807609f18fe42a9820de53c8a0ea1d0a2d5"
       "0f8f5e92a94f00d"},
      {"0x77", "0x3E6C7F3", "0xF385", "0x6091", "0x1A4D1",
       "0x02f86b04778260918301a4d182f38594000000000000000000000000000000000"
       "000aaaa8403e6c7f380c001a05e40977f4064a2bc08785e422ed8a47b56aa219abe9325"
       "1d2b3b4d0cf937b8c0a071e600cd15589c3607bd9627314b99e9b5976bd427b774aa685"
       "bd0d036b1771e"}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    std::vector<uint8_t> private_key;
    EXPECT_TRUE(base::HexStringToBytes(
        "8f2a55949038a9610f50fb23b5883af3b4ecb3c3bb792cbcefbd1542c692be63",
        &private_key));

    HDKey key;
    key.SetPrivateKey(private_key);
    Eip1559Transaction tx =
        *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
            mojom::TxData::New(cases[i].nonce, "0x00", cases[i].gas_limit,
                               "0x000000000000000000000000000000000000aaaa",
                               cases[i].value, std::vector<uint8_t>()),
            "0x04", cases[i].max_priority_fee_per_gas,
            cases[i].max_fee_per_gas));

    int recid;
    const std::vector<uint8_t> signature =
        key.Sign(tx.GetMessageToSign(), &recid);
    tx.ProcessSignature(signature, recid);
    EXPECT_EQ(tx.GetSignedTransaction(), cases[i].signed_tx);
  }
}

TEST(Eip1559TransactionUnitTest, GetUpfrontCost) {
  Eip1559Transaction tx =
      *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
          mojom::TxData::New("0x00", "0x00", "0x64",
                             "0x0101010101010101010101010101010101010101",
                             "0x06", std::vector<uint8_t>()),
          "0x04", "0x8", "0xA"));
  EXPECT_EQ(tx.GetUpfrontCost(), uint256_t(806));
  EXPECT_EQ(tx.GetUpfrontCost(0), uint256_t(806));
  EXPECT_EQ(tx.GetUpfrontCost(4), uint256_t(1006));
}

TEST(Eip1559TransactionUnitTest, Serialization) {
  Eip1559Transaction tx =
      *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0xde0b6b3a7640000", std::vector<uint8_t>()),
          "0x15BE", "0x7B", "0x1C8"));
  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);

  base::Value tx_value = tx.ToValue();
  auto tx_from_value = Eip1559Transaction::FromValue(tx_value);
  ASSERT_NE(tx_from_value, absl::nullopt);
  EXPECT_EQ(*tx_from_value, tx);
}

}  // namespace brave_wallet
