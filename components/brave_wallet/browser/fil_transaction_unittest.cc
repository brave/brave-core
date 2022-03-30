/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_transaction.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

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
  EXPECT_TRUE(first.cid().empty());

  FilTransaction empty;
  EXPECT_EQ(first, empty);

  first.set_cid("cid");
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
  EXPECT_EQ(first.cid(), "cid");

  EXPECT_NE(first, empty);
  auto third = FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid"));
  EXPECT_TRUE(third);
  EXPECT_EQ(third->nonce().value(), 1u);
  EXPECT_EQ(third->gas_premium(), "2");
  EXPECT_EQ(third->gas_fee_cap(), "3");
  EXPECT_EQ(third->gas_limit(), 4u);
  EXPECT_EQ(third->max_fee(), "5");
  EXPECT_EQ(third->to(), address);
  EXPECT_EQ(third->value(), "6");
  EXPECT_EQ(third->cid(), "cid");
  EXPECT_EQ(first, third);
}

TEST(FilTransactionUnitTest, FromTxData) {
  // nonce empty
  auto empty_nonce = FilTransaction::FromTxData(mojom::FilTxData::New(
      "", "2", "3", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "1", "cid"));
  ASSERT_TRUE(empty_nonce);
  EXPECT_FALSE(empty_nonce->nonce());

  // non numeric values
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "a", "2", "3", "4", "d", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "b", "3", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "c", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "d", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "e", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));

  // empty values
  EXPECT_TRUE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "", "2", "3", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_TRUE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "", "3", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_TRUE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_TRUE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "", "", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      mojom::FilTxData::New("1", "2", "3", "4", "5", "", "", "6", "cid")));
  EXPECT_TRUE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "")));

  // invalid address
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "e", "t1h4n7rp3q", "t1h4n7rp2a", "0x1", "cid")));

  // invalid value
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "e", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "0x1", "cid")));
  EXPECT_FALSE(FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "e", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "", "cid")));
}

TEST(FilTransactionUnitTest, Serialization) {
  auto transaction = FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "4", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid"));
  EXPECT_EQ(transaction, FilTransaction::FromValue(transaction->ToValue()));

  auto empty_nonce = FilTransaction::FromTxData(mojom::FilTxData::New(
      "", "2", "3", "1", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", "cid"));
  EXPECT_EQ(empty_nonce, FilTransaction::FromValue(empty_nonce->ToValue()));
}

TEST(FilTransactionUnitTest, GetMessageToSign) {
  auto transaction = FilTransaction::FromTxData(mojom::FilTxData::New(
      "1", "2", "3", "1", "5", "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
      "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6", ""));
  std::string message_to_sign = transaction->GetMessageToSign();
  EXPECT_EQ(message_to_sign,
            "{\"from\":\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\","
            "\"gasfeecap\":\"3\",\"gaslimit\":1,\"gaspremium\":\"2\","
            "\"method\":0,\"nonce\":1,\"params\":\"\",\"to\":"
            "\"t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q\",\"value\":\"6\"}");

  std::string signature = transaction->GetSignedTransaction(
      "8VcW07ADswS4BV2cxi5rnIadVsyTDDhY1NfDH19T8Uo=");
  auto signature_value = base::JSONReader::Read(signature);
  EXPECT_TRUE(signature_value);
  auto* message = signature_value->FindKey("message");
  auto* signature_data = signature_value->FindStringPath("signature.data");
  EXPECT_TRUE(message);
  EXPECT_TRUE(signature_data);
  auto message_as_value = base::JSONReader::Read(message_to_sign);
  EXPECT_TRUE(message_as_value);
  EXPECT_EQ(*message, *message_as_value);
  EXPECT_EQ(*signature_data,
            "SozNIZGNAvALCWtc38OUhO9wdFl82qESGhjnVVhI6CYNN0gP5qa+hZtyFh+"
            "j9K0wIVVU10ZJPgaV0yM6a+xwKgA=");
  // EXPECT_EQ(result.signature, "");
}

}  // namespace brave_wallet
