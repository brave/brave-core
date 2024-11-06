/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip1559_transaction.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

mojom::GasEstimation1559Ptr GetMojomGasEstimation() {
  return mojom::GasEstimation1559::New(
      "0x3b9aca00" /* Hex of 1 * 1e9 */, "0xaf16b1600" /* Hex of 47 * 1e9 */,
      "0x77359400" /* Hex of 2 * 1e9 */, "0xb2d05e000" /* Hex of 48 * 1e9 */,
      "0xb2d05e00" /* Hex of 3 * 1e9 */, "0xb68a0aa00" /* Hex of 49 * 1e9 */,
      "0xad8075b7a" /* Hex of 46574033786 */);
}

}  // namespace

TEST(Eip1559TransactionUnitTest, GetMessageToSign) {
  std::vector<uint8_t> data;
  EXPECT_TRUE(base::HexStringToBytes("010200", &data));
  Eip1559Transaction tx =
      *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
          mojom::TxData::New("0x00", "0x00", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", data, false, std::nullopt),
          "0x04", "0x0", "0x0", nullptr));
  ASSERT_EQ(tx.type(), 2);
  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item;
  item.address.fill(0x01);

  Eip2930Transaction::AccessedStorageKey storage_key_1;
  storage_key_1.fill(0x01);
  item.storage_keys.push_back(storage_key_1);

  access_list->push_back(item);

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.GetHashedMessageToSign(0))),
            "fa81814f7dd57bad435657a05eabdba2815f41e3f15ddd6139027e7db56b0dea");
}

TEST(Eip1559TransactionUnitTest, GetSignedTransactionAndHash) {
  const struct {
    const char* nonce;
    const char* value;
    const char* gas_limit;
    const char* max_priority_fee_per_gas;
    const char* max_fee_per_gas;
    const char* signed_tx;
    const char* hash;
  } cases[] = {
      {"0x333", "0x2933BC9", "0x8AE0", "0x1284D", "0x1D97C",
       "0x02f86e048203338301284d8301d97c828ae094000000000000000000000000000"
       "000000000aaaa8402933bc980c080a00f924cb68412c8f1cfd74d9b581c71eeaf94ff"
       "f6abdde3e5b02ca6b2931dcf47a07dd1c50027c3e31f8b565e25ce68a5072110f61fce5"
       "eee81b195dd51273c2f83",
       "0x2e564c87eb4b40e7f469b2eec5aa5d18b0b46a24e8bf0919439cfb0e8fcae446"},
      {"0x161", "0x3B08B33", "0x7F51", "0x97C2", "0x21467",
       "0x02f86d048201618297c283021467827f519400000000000000000000000000000"
       "0000000aaaa8403b08b3380c080a08caf712f72489da6f1a634b651b4b1c7d9be7d1e"
       "8d05ea76c1eccee3bdfb86a5a06aecc106f588ce51e112f5e9ea7aba3e089dc75117188"
       "21d0e0cd52f52af4e45",
       "0xfc638a8bd28e117cc475935979b36f272cc792ee9adde78ed66df16a72a8cdba"},
      {"0x3D9", "0x1F06571", "0x10BBD", "0x10349", "0x213A1",
       "0x02f86f048203d983010349830213a183010bbd940000000000000000000000000"
       "00000000000aaaa8401f0657180c001a08c03a86e85789ee9a1b42fa0a86d316fca26"
       "2694f8c198df11f194678c2c2d35a028f8e7de02b35014a17b6d28ff8c7e7be6860e726"
       "5ac162fb721f1aeae75643c",
       "0x8b83d04ac346cad8cb062ec81133cede02c103c34b7776e591b430725aa5b0db"},
      {"0x26F", "0x14A5987", "0xE17D", "0x1219C", "0x13D15",
       "0x02f86e0482026f8301219c83013d1582e17d94000000000000000000000000000"
       "000000000aaaa84014a598780c001a0b87c4c8c505d2d692ac77ba466547e79dd60fe"
       "7ecd303d520bf6e8c7085e3182a06dc7d00f5e68c3f3ebe8ae35a90d46051afde620ac1"
       "2e43cae9560a29b13e7fb",
       "0xe45dc03bc114fd3fee8c4d6f3e84b82811e741eeaca0ca5b982a50941be215b3"},
      {"0x3CC", "0x5A2EC37", "0xFEE6", "0xA72E", "0x1942A",
       "0x02f86d048203cc82a72e8301942a82fee69400000000000000000000000000000"
       "0000000aaaa8405a2ec3780c001a006cf07af78c187db104496c58d679f37fcd2d579"
       "0970cecf9a59fe4a5321b375a039f3faafc71479d283a5b1e66a86b19c4bdc516655d89"
       "dbe57d9747747c01dfe",
       "0x41b66fcc516bffb9b1db855e1ceed8ac22c4cec0c81778ca358dee49fc46ab9c"},
      {"0x24C", "0x5EC1B9F", "0x919A", "0x15752", "0x1FCE1",
       "0x02f86e0482024c830157528301fce182919a94000000000000000000000000000"
       "000000000aaaa8405ec1b9f80c080a03e2f59ac9ca852034c2c1da35a742ca19fdd91"
       "0aa5d2ed49ab8ad27a2fcb2b10a03ac1c29c26723c58f91400fb6dfb5f5b837467b1c37"
       "7541b47dae474dddbe469",
       "0xdb703381b81f88b43b5f35a202d9d408a19211fa9f5d90eabb51203a6f445ea9"},
      {"0x384", "0x1CFE6D1", "0x12915", "0x220A", "0x1B841",
       "0x02f86e0482038482220a8301b8418301291594000000000000000000000000000"
       "000000000aaaa8401cfe6d180c001a0f7ffc5bca2512860f8236360159bf303dcfab715"
       "46b6a0032df0306f3739d0c4a05d38fe2c4edebdc1edc157034f780c53a0e5ae089e572"
       "20745bd48bcb10cdf87",
       "0xdae486b05621c1f58b5fed7aebed646dccb8ba24da3627ec30c8c28b75ba3337"},
      {"0x2C5", "0x62D8DB", "0x6EAF", "0x150EC", "0x171AC",
       "0x02f86d048202c5830150ec830171ac826eaf94000000000000000000000000000"
       "000000000aaaa8362d8db80c001a0a61a5710512f346c9996377f7b564ccb64c73a5fdb"
       "615499adb1250498f3e01aa002d10429572cecfaa911a58bbe05f2b26e4c3aee3402202"
       "153a93692849add11",
       "0xa9361a1b88883848872a9140da45e4ef7a029d3699c5703e164414a00536029c"},
      {"0x3AB", "0x2A76B9", "0xAFF7", "0xB0A0", "0x16600",
       "0x02f86c048203ab82b0a08301660082aff79400000000000000000000000000000"
       "0000000aaaa832a76b980c001a0191f0f6667a20cefc0b454e344cc01108aafbdc4e4e5"
       "ed88fdd1b5d108495b31a020879042b0f8d3807609f18fe42a9820de53c8a0ea1d0a2d5"
       "0f8f5e92a94f00d",
       "0xdff2cae411536ce697ad490c5166ea378b9d8d64c5c63c167b4e2cf2319705d0"},
      {"0x77", "0x3E6C7F3", "0xF385", "0x6091", "0x1A4D1",
       "0x02f86b04778260918301a4d182f38594000000000000000000000000000000000"
       "000aaaa8403e6c7f380c001a05e40977f4064a2bc08785e422ed8a47b56aa219abe9325"
       "1d2b3b4d0cf937b8c0a071e600cd15589c3607bd9627314b99e9b5976bd427b774aa685"
       "bd0d036b1771e",
       "0x863c02549182b91f1764714b93d7e882f010539c0907adaf4de761f7b06a713c"}};
  for (const auto& entry : cases) {
    SCOPED_TRACE(entry.signed_tx);
    std::array<uint8_t, 32> private_key;
    EXPECT_TRUE(base::HexStringToSpan(
        "8f2a55949038a9610f50fb23b5883af3b4ecb3c3bb792cbcefbd1542c692be63",
        private_key));

    HDKey key;
    key.SetPrivateKey(private_key);
    Eip1559Transaction tx =
        *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
            mojom::TxData::New(entry.nonce, "0x00", entry.gas_limit,
                               "0x000000000000000000000000000000000000aaaa",
                               entry.value, std::vector<uint8_t>(), false,
                               std::nullopt),
            "0x04", entry.max_priority_fee_per_gas, entry.max_fee_per_gas,
            nullptr));

    int recid;
    auto signature = key.SignCompact(tx.GetHashedMessageToSign(0), &recid);
    ASSERT_TRUE(signature);
    tx.ProcessSignature(*signature, recid, 0);
    EXPECT_EQ(tx.GetSignedTransaction(), entry.signed_tx);
    EXPECT_EQ(tx.GetTransactionHash(), entry.hash);
  }
}

TEST(Eip1559TransactionUnitTest, GetUpfrontCost) {
  Eip1559Transaction tx =
      *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
          mojom::TxData::New("0x00", "0x00", "0x64",
                             "0x0101010101010101010101010101010101010101",
                             "0x06", std::vector<uint8_t>(), false,
                             std::nullopt),
          "0x04", "0x8", "0xA", nullptr));
  EXPECT_EQ(tx.GetUpfrontCost(), uint256_t(806));
  EXPECT_EQ(tx.GetUpfrontCost(0), uint256_t(806));
  EXPECT_EQ(tx.GetUpfrontCost(4), uint256_t(1006));
}

TEST(Eip1559TransactionUnitTest, Serialization) {
  Eip1559Transaction tx =
      *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0xde0b6b3a7640000", std::vector<uint8_t>(), false,
                             std::nullopt),
          "0x15BE", "0x7B", "0x1C8", GetMojomGasEstimation()));

  auto* access_list = tx.access_list();

  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);

  base::Value::Dict tx_value = tx.ToValue();
  auto tx_from_value = Eip1559Transaction::FromValue(tx_value);
  ASSERT_NE(tx_from_value, std::nullopt);
  EXPECT_EQ(*tx_from_value, tx);
}

TEST(Eip1559TransactionUnitTest, FromTxData) {
  auto tx = Eip1559Transaction::FromTxData(mojom::TxData1559::New(
      mojom::TxData::New("0x01", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      "0x15BE", "0x7B", "0x1C8", GetMojomGasEstimation()));
  ASSERT_TRUE(tx);
  EXPECT_EQ(tx->nonce().value(), uint256_t(1));
  EXPECT_EQ(tx->gas_price(), uint256_t(1000));
  EXPECT_EQ(tx->gas_limit(), uint256_t(10000000));
  EXPECT_EQ(tx->to(),
            EthAddress::FromHex("0x3535353535353535353535353535353535353535"));
  EXPECT_EQ(tx->value(), uint256_t(42));
  EXPECT_EQ(tx->data(), std::vector<uint8_t>{1});
  EXPECT_EQ(tx->chain_id(), uint256_t(5566));
  EXPECT_EQ(tx->max_priority_fee_per_gas(), uint256_t(123));
  EXPECT_EQ(tx->max_fee_per_gas(), uint256_t(456));
  EXPECT_EQ(tx->gas_estimation().slow_max_priority_fee_per_gas, uint256_t(1e9));
  EXPECT_EQ(tx->gas_estimation().avg_max_priority_fee_per_gas,
            uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx->gas_estimation().fast_max_priority_fee_per_gas,
            uint256_t(3) * uint256_t(1e9));
  EXPECT_EQ(tx->gas_estimation().slow_max_fee_per_gas,
            uint256_t(47) * uint256_t(1e9));
  EXPECT_EQ(tx->gas_estimation().avg_max_fee_per_gas,
            uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx->gas_estimation().fast_max_fee_per_gas,
            uint256_t(49) * uint256_t(1e9));
  EXPECT_EQ(tx->gas_estimation().base_fee_per_gas, uint256_t(46574033786ULL));

  // Empty nonce should succeed.
  tx = Eip1559Transaction::FromTxData(mojom::TxData1559::New(
      mojom::TxData::New("", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      "0x15BE", "0x7B", "0x1C8", nullptr));
  ASSERT_TRUE(tx);
  EXPECT_FALSE(tx->nonce());

  // Invalid nonce should fail.
  EXPECT_FALSE(Eip1559Transaction::FromTxData(mojom::TxData1559::New(
      mojom::TxData::New("123", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      "0x15BE", "0x7B", "0x1C8", nullptr)));

  // Make sure chain id, and the max priority fee fields must all have
  // fields when strict is true
  EXPECT_FALSE(Eip1559Transaction::FromTxData(mojom::TxData1559::New(
      mojom::TxData::New("0x1", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      "", "0x7B", "0x1C8", nullptr)));
  EXPECT_FALSE(Eip1559Transaction::FromTxData(mojom::TxData1559::New(
      mojom::TxData::New("0x1", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      "0x15BE", "", "0x1C8", nullptr)));
  EXPECT_FALSE(Eip1559Transaction::FromTxData(mojom::TxData1559::New(
      mojom::TxData::New("0x1", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      "0x15BE", "0x7B", "", nullptr)));

  // But missing data is allowed when strict is false
  tx = Eip1559Transaction::FromTxData(
      mojom::TxData1559::New(
          mojom::TxData::New("", "0x3E8", "",
                             "0x3535353535353535353535353535353535353535", "",
                             std::vector<uint8_t>{1}, false, std::nullopt),
          "", "0x7B", "0x1C8", nullptr),
      false);
  ASSERT_TRUE(tx);
  // Empty nonce will be std::nullopt
  EXPECT_FALSE(tx->nonce());
  // Unspecified value defaults to 0
  EXPECT_EQ(tx->gas_limit(), uint256_t(0));
  EXPECT_EQ(tx->value(), uint256_t(0));

  // Unspecified gas estimation will be default values.
  EXPECT_EQ(tx->gas_estimation(), Eip1559Transaction::GasEstimation());

  // you can still get at other data that is specified
  EXPECT_EQ(tx->gas_price(), uint256_t(1000));
  EXPECT_EQ(tx->chain_id(), uint256_t(0));
  EXPECT_EQ(tx->max_priority_fee_per_gas(), uint256_t(123));
  EXPECT_EQ(tx->max_fee_per_gas(), uint256_t(456));

  // Other fields are missing
  tx = Eip1559Transaction::FromTxData(
      mojom::TxData1559::New(
          mojom::TxData::New("", "0x3E8", "",
                             "0x3535353535353535353535353535353535353535", "",
                             std::vector<uint8_t>{1}, false, std::nullopt),
          "0x15BE", "", "", nullptr),
      false);
  ASSERT_TRUE(tx);
  // Empty nonce will be std::nullopt
  EXPECT_FALSE(tx->nonce());
  // Unspecified value defaults to 0
  EXPECT_EQ(tx->gas_limit(), uint256_t(0));
  EXPECT_EQ(tx->value(), uint256_t(0));
  EXPECT_EQ(tx->max_priority_fee_per_gas(), uint256_t(0));
  EXPECT_EQ(tx->max_fee_per_gas(), uint256_t(0));
  // you can still get at other data that is specified
  EXPECT_EQ(tx->gas_price(), uint256_t(1000));
  EXPECT_EQ(tx->chain_id(), uint256_t(5566));

  // Default gas estimation values (0) will be used if any fields in gas
  // estimation struct is missing regardless of the value of strict.
  auto missing_fields_gas_estimation = GetMojomGasEstimation();
  missing_fields_gas_estimation->avg_max_priority_fee_per_gas = "";

  tx = Eip1559Transaction::FromTxData(mojom::TxData1559::New(
      mojom::TxData::New("0x01", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      "0x15BE", "0x7B", "0x1C8", missing_fields_gas_estimation.Clone()));
  EXPECT_EQ(tx->gas_estimation(), Eip1559Transaction::GasEstimation());

  tx = Eip1559Transaction::FromTxData(
      mojom::TxData1559::New(
          mojom::TxData::New("0x01", "0x3E8", "0x989680",
                             "0x3535353535353535353535353535353535353535",
                             "0x2A", std::vector<uint8_t>{1}, false,
                             std::nullopt),
          "0x15BE", "0x7B", "0x1C8", missing_fields_gas_estimation.Clone()),
      false);
  EXPECT_EQ(tx->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST(Eip1559TransactionUnitTest, GasEstimationFromMojomGasEstimation1559) {
  auto estimation =
      Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
          GetMojomGasEstimation());
  EXPECT_TRUE(estimation);
  EXPECT_EQ(estimation->slow_max_priority_fee_per_gas, uint256_t(1e9));
  EXPECT_EQ(estimation->avg_max_priority_fee_per_gas,
            uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(estimation->fast_max_priority_fee_per_gas,
            uint256_t(3) * uint256_t(1e9));
  EXPECT_EQ(estimation->slow_max_fee_per_gas, uint256_t(47) * uint256_t(1e9));
  EXPECT_EQ(estimation->avg_max_fee_per_gas, uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(estimation->fast_max_fee_per_gas, uint256_t(49) * uint256_t(1e9));
  EXPECT_EQ(estimation->base_fee_per_gas, uint256_t(46574033786ULL));

  auto mojom_gas_estimation = GetMojomGasEstimation();
  mojom_gas_estimation->slow_max_priority_fee_per_gas = "123";
  EXPECT_FALSE(Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
      std::move(mojom_gas_estimation)));

  mojom_gas_estimation = GetMojomGasEstimation();
  mojom_gas_estimation->avg_max_priority_fee_per_gas = "";
  EXPECT_FALSE(Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
      std::move(mojom_gas_estimation)));
}

TEST(Eip1559TransactionUnitTest, GasEstimationToMojomGasEstimation1559) {
  Eip1559Transaction::GasEstimation estimation;
  estimation.slow_max_priority_fee_per_gas = uint256_t(1e9);
  estimation.avg_max_priority_fee_per_gas = uint256_t(2) * uint256_t(1e9);
  estimation.fast_max_priority_fee_per_gas = uint256_t(3) * uint256_t(1e9);
  estimation.slow_max_fee_per_gas = uint256_t(47) * uint256_t(1e9);
  estimation.avg_max_fee_per_gas = uint256_t(48) * uint256_t(1e9);
  estimation.fast_max_fee_per_gas = uint256_t(49) * uint256_t(1e9);
  estimation.base_fee_per_gas = uint256_t(46574033786ULL);
  EXPECT_EQ(
      Eip1559Transaction::GasEstimation::ToMojomGasEstimation1559(estimation),
      GetMojomGasEstimation());
}

}  // namespace brave_wallet
