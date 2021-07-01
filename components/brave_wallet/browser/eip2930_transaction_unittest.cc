/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/hd_key.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
TEST(Eip2930TransactionUnitTest, GetMessageToSign) {
  std::vector<uint8_t> data;
  EXPECT_TRUE(base::HexStringToBytes("010200", &data));
  EthTransaction::TxData tx_data(
      0x00, 0x00, 0x00,
      EthAddress::FromHex("0x0101010101010101010101010101010101010101"), 0x00,
      data);
  Eip2930Transaction tx(tx_data, 0x01);
  ASSERT_EQ(tx.type(), 1);
  auto* access_list = tx.access_list();
  Eip2930Transaction::AccessListItem item;
  item.address.fill(0x01);

  Eip2930Transaction::AccessedStorageKey storage_key_1;
  storage_key_1.fill(0x01);
  item.storage_keys.push_back(storage_key_1);

  access_list->push_back(item);

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.GetMessageToSign())),
            "78528e2724aa359c58c13e43a7c467eb721ce8d410c2a12ee62943a3aaefb60b");
}

TEST(Eip2930TransactionUnitTest, GetSignedTransaction) {
  EthTransaction::TxData tx_data(
      0x00, 0x3b9aca00, 0x62d4,
      EthAddress::FromHex("0xdf0a88b2b68c673713a8ec826003676f272e3573"), 0x01,
      std::vector<uint8_t>());
  Eip2930Transaction tx(tx_data, 0x796f6c6f763378);
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

  std::vector<uint8_t> private_key;
  EXPECT_TRUE(base::HexStringToBytes(
      "fad9c8855b740a0b7ed4c221dbad0f33a83a49cad6b3fe8d5817ac83d38b6a19",
      &private_key));

  HDKey key;
  key.SetPrivateKey(private_key);
  int recid;
  const std::vector<uint8_t> signature =
      key.Sign(tx.GetMessageToSign(), &recid);

  ASSERT_FALSE(tx.IsSigned());
  tx.ProcessSignature(signature, recid);
  ASSERT_TRUE(tx.IsSigned());
  EXPECT_EQ(
      tx.GetSignedTransaction(),
      "0x01f8a587796f6c6f76337880843b9aca008262d494df0a88b2b68c673713a8ec826003"
      "676f272e35730180f838f7940000000000000000000000000000000000001337e1a00000"
      "00000000000000000000000000000000000000000000000000000000000080a0294ac940"
      "77b35057971e6b4b06dfdf55a6fbed819133a6c1d31e187f1bca938da00be950468ba1c2"
      "5a5cb50e9f6d8aa13c8cd21f24ba909402775b262ac76d374d");

  EXPECT_EQ(tx.v_, 0u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.r_)),
            "294ac94077b35057971e6b4b06dfdf55a6fbed819133a6c1d31e187f1bca938d");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.s_)),
            "0be950468ba1c25a5cb50e9f6d8aa13c8cd21f24ba909402775b262ac76d374d");
}
}  // namespace brave_wallet
