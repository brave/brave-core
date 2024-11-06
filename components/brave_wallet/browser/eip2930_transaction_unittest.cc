/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip2930_transaction.h"

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

TEST(Eip2930TransactionUnitTest, AccessListItemEqualOperator) {
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  Eip2930Transaction::AccessedStorageKey storage_key_1;
  storage_key_1.fill(0x01);
  item_a.storage_keys.push_back(storage_key_0);
  item_a.storage_keys.push_back(storage_key_1);

  Eip2930Transaction::AccessListItem item_b;
  item_b.address.fill(0x0a);
  item_b.storage_keys.push_back(storage_key_0);
  item_b.storage_keys.push_back(storage_key_1);
  EXPECT_EQ(item_a, item_b);

  Eip2930Transaction::AccessListItem item_c(item_b);
  item_c.address.fill(0x0b);
  EXPECT_NE(item_a, item_c);

  Eip2930Transaction::AccessListItem item_d(item_b);
  item_d.storage_keys.push_back(storage_key_0);
  EXPECT_NE(item_a, item_d);

  Eip2930Transaction::AccessListItem item_e(item_b);
  item_e.storage_keys[0].fill(0x03);
  EXPECT_NE(item_a, item_e);
}

TEST(Eip2930TransactionUnitTest, AccessListAndValue) {
  Eip2930Transaction::AccessList access_list;
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  Eip2930Transaction::AccessedStorageKey storage_key_1;
  storage_key_1.fill(0x01);
  item_a.storage_keys.push_back(storage_key_0);
  item_a.storage_keys.push_back(storage_key_1);

  access_list.push_back(item_a);

  Eip2930Transaction::AccessListItem item_b;
  item_b.address.fill(0x0b);
  item_b.storage_keys.push_back(storage_key_1);
  item_b.storage_keys.push_back(storage_key_0);

  access_list.push_back(item_b);

  base::Value access_list_value =
      base::Value(Eip2930Transaction::AccessListToValue(access_list));
  auto access_list_from_value =
      Eip2930Transaction::ValueToAccessList(access_list_value.GetList());
  ASSERT_NE(access_list_from_value, std::nullopt);
  EXPECT_EQ(*access_list_from_value, access_list);
}

TEST(Eip2930TransactionUnitTest, GetMessageToSign) {
  std::vector<uint8_t> data;
  EXPECT_TRUE(base::HexStringToBytes("010200", &data));
  Eip2930Transaction tx = *Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x00", "0x00", "0x00",
                         "0x0101010101010101010101010101010101010101", "0x00",
                         data, false, std::nullopt),
      0x01);
  ASSERT_EQ(tx.type(), 1);
  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item;
  item.address.fill(0x01);

  Eip2930Transaction::AccessedStorageKey storage_key_1;
  storage_key_1.fill(0x01);
  item.storage_keys.push_back(storage_key_1);

  access_list->push_back(item);

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.GetHashedMessageToSign(0))),
            "78528e2724aa359c58c13e43a7c467eb721ce8d410c2a12ee62943a3aaefb60b");
}

TEST(Eip2930TransactionUnitTest, GetSignedTransactionAndHash) {
  Eip2930Transaction tx = *Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x00", "0x3b9aca00", "0x62d4",
                         "0xdf0a88b2b68c673713a8ec826003676f272e3573", "0x01",
                         std::vector<uint8_t>(), false, std::nullopt),
      0x796f6c6f763378);
  ASSERT_EQ(tx.type(), 1);
  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item;
  std::vector<uint8_t> address;
  ASSERT_TRUE(base::HexStringToBytes("0000000000000000000000000000000000001337",
                                     &address));
  std::move(address.begin(), address.end(), item.address.begin());

  Eip2930Transaction::AccessedStorageKey storage_key_1;
  storage_key_1.fill(0x00);
  item.storage_keys.push_back(storage_key_1);

  access_list->push_back(item);

  std::array<uint8_t, 32> private_key;
  EXPECT_TRUE(base::HexStringToSpan(
      "fad9c8855b740a0b7ed4c221dbad0f33a83a49cad6b3fe8d5817ac83d38b6a19",
      private_key));

  HDKey key;
  key.SetPrivateKey(private_key);
  int recid;
  auto signature = key.SignCompact(tx.GetHashedMessageToSign(0), &recid);
  ASSERT_TRUE(signature);

  ASSERT_FALSE(tx.IsSigned());
  tx.ProcessSignature(*signature, recid, 0);
  ASSERT_TRUE(tx.IsSigned());
  EXPECT_EQ(
      tx.GetSignedTransaction(),
      "0x01f8a587796f6c6f76337880843b9aca008262d494df0a88b2b68c673713a8ec826003"
      "676f272e35730180f838f7940000000000000000000000000000000000001337e1a00000"
      "00000000000000000000000000000000000000000000000000000000000080a0294ac940"
      "77b35057971e6b4b06dfdf55a6fbed819133a6c1d31e187f1bca938da00be950468ba1c2"
      "5a5cb50e9f6d8aa13c8cd21f24ba909402775b262ac76d374d");
  EXPECT_EQ(
      tx.GetTransactionHash(),
      "0xbbd570a3c6acc9bb7da0d5c0322fe4ea2a300db80226f7df4fef39b2d6649eec");

  EXPECT_EQ(tx.v_, (uint256_t)0);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.r_)),
            "294ac94077b35057971e6b4b06dfdf55a6fbed819133a6c1d31e187f1bca938d");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.s_)),
            "0be950468ba1c25a5cb50e9f6d8aa13c8cd21f24ba909402775b262ac76d374d");
}

TEST(Eip2930TransactionUnitTest, Serialization) {
  Eip2930Transaction tx = *Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                         "0x3535353535353535353535353535353535353535",
                         "0x0de0b6b3a7640000", std::vector<uint8_t>(), false,
                         std::nullopt),
      0x15BE);
  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);

  base::Value::Dict tx_value = tx.ToValue();
  auto tx_from_value = Eip2930Transaction::FromValue(tx_value);
  ASSERT_NE(tx_from_value, std::nullopt);
  EXPECT_EQ(tx_from_value, tx);
}

TEST(Eip2930TransactionUnitTest, GetBaseFee) {
  std::vector<uint8_t> data;
  ASSERT_TRUE(base::HexStringToBytes("010200", &data));
  Eip2930Transaction tx = *Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x0", "0x0", "0x0",
                         "0x3535353535353535353535353535353535353535", "0x0",
                         data, false, std::nullopt),
      0x15BE);

  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);

  // Tx cost + 2*TxDataNonZero + TxDataZero + AccessListAddressCost +
  // AccessListSlotCost
  const uint256_t fee = 21000 + 2 * 16 + 4 + 2400 + 1900;
  EXPECT_EQ(tx.GetBaseFee(), fee);

  Eip2930Transaction tx2 = *Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x0", "0x0", "0x0", "", "0x0", data, false,
                         std::nullopt),
      0x15BE);
  *tx2.access_list() = *tx.access_list();
  // Plus contract creation
  const uint256_t fee2 = fee + uint256_t(32000);
  EXPECT_EQ(tx2.GetBaseFee(), fee2);

  // Duplicate items in Access list
  Eip2930Transaction tx3 = *Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x0", "0x0", "0x0",
                         "0x3535353535353535353535353535353535353535", "0x0",
                         std::vector<uint8_t>(), false, std::nullopt),
      0x15BE);

  auto* access_list3 = tx3.access_list();
  access_list3->push_back(item_a);
  Eip2930Transaction::AccessListItem item_b(item_a);
  item_b.storage_keys.push_back(storage_key_0);
  access_list3->push_back(item_b);

  const uint256_t fee3 = 21000 + 2 * 2400 + 3 * 1900;
  EXPECT_EQ(tx3.GetBaseFee(), fee3);
}

TEST(Eip2930TransactionUnitTest, FromTxData) {
  auto tx = Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x01", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      1);
  ASSERT_TRUE(tx);
  EXPECT_EQ(tx->nonce(), uint256_t(1));
  EXPECT_EQ(tx->gas_price(), uint256_t(1000));
  EXPECT_EQ(tx->gas_limit(), uint256_t(10000000));
  EXPECT_EQ(tx->to(),
            EthAddress::FromHex("0x3535353535353535353535353535353535353535"));
  EXPECT_EQ(tx->value(), uint256_t(42));
  EXPECT_EQ(tx->data(), std::vector<uint8_t>{1});
  EXPECT_EQ(tx->chain_id(), uint256_t(1));

  // Empty nonce
  tx = Eip2930Transaction::FromTxData(
      mojom::TxData::New("", "0x3E8", "0x989680",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      0);
  ASSERT_TRUE(tx);
  EXPECT_FALSE(tx->nonce());

  // Simplified just test cases since EthTransaction has detailed tests for a
  // single missing value
  EXPECT_FALSE(Eip2930Transaction::FromTxData(
      mojom::TxData::New("0x1", "0x3E8", "",
                         "0x3535353535353535353535353535353535353535", "0x2A",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      0));

  // But missing data is allowed when strict is false
  tx = Eip2930Transaction::FromTxData(
      mojom::TxData::New("", "0x3E8", "",
                         "0x3535353535353535353535353535353535353535", "",
                         std::vector<uint8_t>{1}, false, std::nullopt),
      1, false);
  ASSERT_TRUE(tx);
  // Empty nonce should be nullopt
  EXPECT_FALSE(tx->nonce());
  // Unspecified value defaults to 0
  EXPECT_EQ(tx->gas_limit(), uint256_t(0));
  EXPECT_EQ(tx->value(), uint256_t(0));
  // you can still get at other data that is specified
  EXPECT_EQ(tx->gas_price(), uint256_t(1000));
  EXPECT_EQ(tx->chain_id(), uint256_t(1));
}

}  // namespace brave_wallet
