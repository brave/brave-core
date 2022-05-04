/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_transaction.h"

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

TEST(FilTransactionUnitTest, Initialization) {
  FilTransaction first;
  EXPECT_FALSE(first.nonce());
  EXPECT_TRUE(first.gas_premium().empty());
  EXPECT_TRUE(first.gas_fee_cap().empty());
  EXPECT_TRUE(first.max_fee().empty());
  EXPECT_EQ(first.gas_limit(), 0u);
  EXPECT_TRUE(first.to().IsEmpty());
  EXPECT_TRUE(first.value().empty());

  FilTransaction empty;
  EXPECT_EQ(first, empty);

  first.set_nonce(1);
  first.set_gas_premium("2");
  first.set_fee_cap("3");
  first.set_gas_limit(4);
  first.set_max_fee("5");
  auto address =
      FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  first.set_to(address);
  first.set_value("6");

  EXPECT_EQ(first.nonce().value(), 1u);
  EXPECT_EQ(first.gas_premium(), "2");
  EXPECT_EQ(first.gas_fee_cap(), "3");
  EXPECT_EQ(first.gas_limit(), 4u);
  EXPECT_EQ(first.max_fee(), "5");
  EXPECT_EQ(first.to(), address);
  EXPECT_EQ(first.value(), "6");

  EXPECT_NE(first, empty);
  auto third = FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  EXPECT_TRUE(third);
  EXPECT_EQ(third->nonce().value(), 1u);
  EXPECT_EQ(third->gas_premium(), "2");
  EXPECT_EQ(third->gas_fee_cap(), "3");
  EXPECT_EQ(third->gas_limit(), 4u);
  EXPECT_EQ(third->max_fee(), "5");
  EXPECT_EQ(third->to(), address);
  EXPECT_EQ(third->value(), "6");
  EXPECT_EQ(first, third);
}

TEST(FilTransactionUnitTest, FromTxData) {
  // nonce empty
  auto empty_nonce = FilTransaction::FromTxData(
      mojom::FilTxData::New("", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "1"));
  ASSERT_TRUE(empty_nonce);
  EXPECT_FALSE(empty_nonce->nonce());

  // non numeric values
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("a", "2", "3", "4", "d",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "b", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "c", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "d", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "e",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));

  // empty values
  EXPECT_TRUE(FilTransaction::FromTxData(
      mojom::FilTxData::New("", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "", "",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "5", "", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));

  // invalid address
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "e", "t1h4n7rp3q", "0x1")));

  // invalid value
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "e", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "0x1")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "e",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "")));
}

TEST(FilTransactionUnitTest, Serialization) {
  auto transaction = FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  EXPECT_EQ(transaction, FilTransaction::FromValue(transaction->ToValue()));

  auto empty_nonce = FilTransaction::FromTxData(
      mojom::FilTxData::New("", "2", "3", "1", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  EXPECT_EQ(empty_nonce, FilTransaction::FromValue(empty_nonce->ToValue()));
}

TEST(FilTransactionUnitTest, GetMessageToSign) {
  auto transaction = FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "1", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  EXPECT_EQ(transaction->GetMessageToSign(),
            "{\"gas_fee_cap\":\"3\",\"gas_limit\":\"1\",\"gas_"
            "premium\":\"2\",\"max_fee\":\"5\",\"nonce\":\"1\",\"to\":"
            "\"t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q\",\"value\":\"6\"}");
}

TEST(FilTransactionUnitTest, ToFilTxData) {
  auto tx_data =
      mojom::FilTxData::New("1", "2", "3", "1", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6");
  auto transaction = FilTransaction::FromTxData(tx_data);
  EXPECT_EQ(transaction->ToFilTxData(), tx_data);
}

}  // namespace brave_wallet
