/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_transaction.h"

#include <memory>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/services/brave_wallet/public/cpp/test_support/in_process_third_party_service_test_environment.h"
#include "brave/components/services/brave_wallet/public/cpp/third_party_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

void CompareJSONs(const std::string& current_string,
                  const std::string& expected_string) {
  auto current_json = base::JSONReader::Read(current_string);
  ASSERT_TRUE(current_json);
  auto expected_string_json = base::JSONReader::Read(expected_string);
  ASSERT_TRUE(expected_string_json);
  EXPECT_EQ(*current_json, *expected_string_json);
}

std::string DecodePrivateKey(const std::string& private_key_base64) {
  std::string private_key_decoded;
  EXPECT_TRUE(base::Base64Decode(private_key_base64, &private_key_decoded));
  return private_key_decoded;
}

}  // namespace

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
  auto to =
      FilAddress::FromAddress("t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  first.set_to(to);
  first.set_value("6");

  EXPECT_EQ(first.nonce().value(), 1u);
  EXPECT_EQ(first.gas_premium(), "2");
  EXPECT_EQ(first.gas_fee_cap(), "3");
  EXPECT_EQ(first.gas_limit(), 4u);
  EXPECT_EQ(first.max_fee(), "5");
  EXPECT_EQ(first.to(), to);
  EXPECT_EQ(first.value(), "6");

  EXPECT_NE(first, empty);
  auto third = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "5", to.EncodeAsString(), "6"));
  EXPECT_TRUE(third);
  EXPECT_EQ(third->nonce().value(), 1u);
  EXPECT_EQ(third->gas_premium(), "2");
  EXPECT_EQ(third->gas_fee_cap(), "3");
  EXPECT_EQ(third->gas_limit(), 4u);
  EXPECT_EQ(third->max_fee(), "5");
  EXPECT_EQ(third->to(), to);
  EXPECT_EQ(third->value(), "6");
  EXPECT_EQ(first, *third);
}

TEST(FilTransactionUnitTest, FromTxDataEVMAddress) {
  auto first = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", "6"));
  EXPECT_TRUE(first);
  EXPECT_EQ(first->to().EncodeAsString(),
            "t410fwsziaijja4nsxhv3rs5qd2q6juklgslb5opkb5q");

  auto second = FilTransaction::FromTxData(
      true,
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961", "6"));
  EXPECT_TRUE(second);
  EXPECT_EQ(second->to().EncodeAsString(),
            "f410fwsziaijja4nsxhv3rs5qd2q6juklgslb5opkb5q");

  auto third = FilTransaction::FromTxData(
      true, mojom::FilTxData::New("1", "2", "3", "4", "5",
                                  "0xB4B2802129071b2B9eBb8cBB01EA1E4D", "6"));
  EXPECT_FALSE(third);
}

TEST(FilTransactionUnitTest, FromTxData) {
  // nonce empty
  auto empty_nonce = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "1"));
  ASSERT_TRUE(empty_nonce);
  EXPECT_FALSE(empty_nonce->nonce());

  // non numeric values
  EXPECT_FALSE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("a", "2", "3", "4", "d",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "b", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "c", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "d", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "e",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));

  // empty values
  EXPECT_TRUE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "", "",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      false, mojom::FilTxData::New("1", "2", "3", "4", "5", "", "6")));
  EXPECT_TRUE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));

  // invalid address
  EXPECT_FALSE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "e", "t1h4n7rp3q", "0x1")));

  // invalid value
  EXPECT_FALSE(FilTransaction::FromTxData(
      false, mojom::FilTxData::New("1", "2", "3", "4", "e",
                                   "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                                   "0x1")));
  EXPECT_FALSE(FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "e",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "")));
}

TEST(FilTransactionUnitTest, ToFilTxData) {
  auto tx_data =
      mojom::FilTxData::New("1", "2", "3", "1", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6");
  auto transaction = FilTransaction::FromTxData(false, tx_data);
  EXPECT_EQ(transaction->ToFilTxData(), tx_data);
}

TEST(FilTransactionUnitTest, Serialization) {
  auto transaction = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  EXPECT_EQ(transaction, FilTransaction::FromValue(transaction->ToValue()));

  auto empty_nonce = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("", "2", "3", "1", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  EXPECT_EQ(empty_nonce, FilTransaction::FromValue(empty_nonce->ToValue()));
}

class FilSignTransactionUnitTest : public testing::Test {
 public:
  FilSignTransactionUnitTest() = default;
  ~FilSignTransactionUnitTest() override = default;

  void GetSignedTransaction(const FilTransaction& transaction,
                            const FilAddress& from,
                            const std::vector<uint8_t>& private_key,
                            absl::optional<std::string>* signature_out) {
    ASSERT_TRUE(signature_out);

    base::RunLoop run_loop;
    transaction.GetSignedTransaction(
        from, private_key,
        base::BindLambdaForTesting(
            [&run_loop, &signature_out](absl::optional<std::string> signature) {
              *signature_out = signature;
              run_loop.Quit();
            }));
    run_loop.Run();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  InProcessThirdPartyServiceTestEnvironment in_proc_third_party_service_env_;
};

TEST_F(FilSignTransactionUnitTest, GetMessageToSignSecp) {
  auto from =
      FilAddress::FromAddress("t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq");
  auto transaction = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("", "2", "3", "1", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  auto message_to_sign = transaction->GetMessageToSignJson(from);
  ASSERT_TRUE(message_to_sign);
  CompareJSONs(*message_to_sign,
               R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 0,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })");
  transaction->set_nonce(1);
  message_to_sign = transaction->GetMessageToSignJson(from);
  ASSERT_TRUE(message_to_sign);
  CompareJSONs(*message_to_sign,
               R"({
                 "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                 "GasFeeCap": "3",
                 "GasLimit": 1,
                 "GasPremium": "2",
                 "Method": 0,
                 "Params": "",
                 "Nonce": 1,
                 "To": "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                 "Value": "6",
                 "Version": 0
               })");

  std::string private_key_decoded =
      DecodePrivateKey("8VcW07ADswS4BV2cxi5rnIadVsyTDDhY1NfDH19T8Uo=");
  std::vector<uint8_t> private_key(private_key_decoded.begin(),
                                   private_key_decoded.end());
  absl::optional<std::string> signature;
  GetSignedTransaction(*transaction, from, private_key, &signature);
  ASSERT_TRUE(signature.has_value());

  auto signature_value = base::JSONReader::Read(*signature);
  EXPECT_TRUE(signature_value);
  auto* message = signature_value->GetDict().Find("Message");
  auto* signature_data =
      signature_value->GetDict().FindStringByDottedPath("Signature.Data");
  EXPECT_TRUE(message);
  EXPECT_TRUE(signature_data);
  auto message_as_value = base::JSONReader::Read(*message_to_sign);
  EXPECT_TRUE(message_as_value);
  EXPECT_EQ(*signature_data,
            "SozNIZGNAvALCWtc38OUhO9wdFl82qESGhjnVVhI6CYNN0gP5qa+hZtyFh+"
            "j9K0wIVVU10ZJPgaV0yM6a+xwKgA=");
  EXPECT_EQ(*message, *message_as_value);
  auto signature_type =
      signature_value->GetDict().FindIntByDottedPath("Signature.Type");
  ASSERT_TRUE(signature_type);
  EXPECT_EQ(signature_type, 1);
}

TEST_F(FilSignTransactionUnitTest, GetMessageToSignBLS) {
  const std::string from_account =
      "t3uylp7xgte6rpiqhpivxohtzs7okpnq44mnckimwf6mgi6yc4o6f3iyd426u6wzloiig3a4"
      "ocyug4ftz64xza";
  const std::string to_account =
      "t3uylp7xgte6rpiqhpivxohtzs7okpnq44mnckimwf6mgi6yc4o6f3iyd426u6wzloiig3a4"
      "ocyug4ftz64xza";
  auto transaction = FilTransaction::FromTxData(
      false, mojom::FilTxData::New("1", "2", "3", "1", "5", to_account, "6"));
  auto message_to_sign =
      transaction->GetMessageToSignJson(FilAddress::FromAddress(from_account));
  ASSERT_TRUE(message_to_sign);
  std::string expected_message =
      R"({
        "From": "{from_account}",
        "GasFeeCap": "3",
        "GasLimit": 1,
        "GasPremium": "2",
        "Method": 0,
        "Params": "",
        "Nonce": 1,
        "To": "{to_account}",
        "Value": "6",
        "Version": 0
      })";
  base::ReplaceFirstSubstringAfterOffset(&expected_message, 0, "{from_account}",
                                         from_account);
  base::ReplaceFirstSubstringAfterOffset(&expected_message, 0, "{to_account}",
                                         to_account);
  CompareJSONs(*message_to_sign, expected_message);
  std::string private_key_decoded =
      DecodePrivateKey("7ug8i7Q6xddnBpvjbHe8zm+UekV+EVtOUxpNXr+PpCc=");

  std::vector<uint8_t> private_key(private_key_decoded.begin(),
                                   private_key_decoded.end());
  absl::optional<std::string> signature;
  GetSignedTransaction(*transaction, FilAddress::FromAddress(from_account),
                       private_key, &signature);
  ASSERT_TRUE(signature.has_value());

  auto signature_value = base::JSONReader::Read(*signature);
  EXPECT_TRUE(signature_value);
  auto* message = signature_value->GetDict().Find("Message");
  auto* signature_data =
      signature_value->GetDict().FindStringByDottedPath("Signature.Data");
  EXPECT_TRUE(message);
  EXPECT_TRUE(signature_data);
  auto message_as_value = base::JSONReader::Read(*message_to_sign);
  EXPECT_TRUE(message_as_value);
  EXPECT_EQ(
      *signature_data,
      "lsMyTOOAaW9/FxIKupqypmUl1hXLOKrbcJdQs+bHMPNF6aaCu2MaIRQKjS/"
      "Hi6pMB84syUMuxRPC5JdpFvMl7gy5J2kvOEuDclSvc1ALQf2wOalPUOH022DNgLVATD36");
  EXPECT_EQ(*message, *message_as_value);

  auto signature_type =
      signature_value->GetDict().FindIntByDottedPath("Signature.Type");
  ASSERT_TRUE(signature_type);
  EXPECT_EQ(signature_type, 2);
}

}  // namespace brave_wallet
