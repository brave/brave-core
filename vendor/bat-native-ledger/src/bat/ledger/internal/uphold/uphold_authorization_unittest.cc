/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <tuple>
#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_authorization.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=UpholdAuthorization*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger {
namespace uphold {

using GetAnonFundsTestParamType =
    std::tuple<bool,               // fetch old balance enabled
               std::string,        // Brave wallet
               type::UrlResponse,  // balance server response
               std::pair<type::Result,
                         base::Optional<double>>  // expected result
               >;

class GetAnonFundsTest : public TestWithParam<GetAnonFundsTestParamType> {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<database::MockDatabase> mock_database_;
  std::unique_ptr<UpholdAuthorization> authorization_;

 public:
  GetAnonFundsTest()
      : mock_ledger_client_{std::make_unique<MockLedgerClient>()},
        mock_ledger_impl_{
            std::make_unique<MockLedgerImpl>(mock_ledger_client_.get())},
        mock_database_{
            std::make_unique<database::MockDatabase>(mock_ledger_impl_.get())},
        authorization_{
            std::make_unique<UpholdAuthorization>(mock_ledger_impl_.get())} {}
};

std::string GetAnonFundsNameSuffixGenerator(
    const TestParamInfo<GetAnonFundsTest::ParamType>& info) {
  const bool fetch_old_balance_enabled = std::get<0>(info.param);
  const std::string& brave_wallet = std::get<1>(info.param);
  const type::UrlResponse& balance_server_response = std::get<2>(info.param);

  if (!fetch_old_balance_enabled) {
    return "FetchOldBalanceDisabled";
  }

  if (brave_wallet.empty()) {
    return "BraveWalletIsNotCreated";
  }

  if (brave_wallet.find(R"("payment_id": "")") != std::string::npos) {
    return "BraveWalletPaymentIdIsEmpty";
  }

  if (balance_server_response.status_code != net::HttpStatusCode::HTTP_OK) {
    return "BalanceServerError";
  }

  if (balance_server_response.body.empty()) {
    return "InvalidBodyInBalanceServerResponse";
  }

  return "HappyPath";
}

INSTANTIATE_TEST_SUITE_P(
    UpholdAuthorization,
    GetAnonFundsTest,
    Values(
        // "fetch_old_balance_disabled"
        GetAnonFundsTestParamType{false,
                                  {},
                                  {},
                                  {type::Result::LEDGER_OK, 0.0}},
        // "brave_wallet_is_not_created"
        GetAnonFundsTestParamType{true, {}, {}, {type::Result::LEDGER_OK, 0.0}},
        // "brave_wallet_payment_id_is_empty"
        GetAnonFundsTestParamType{
            true,
            R"({ "payment_id": "", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
            {},
            {type::Result::LEDGER_ERROR, {}}},
        // "balance_server_error"
        GetAnonFundsTestParamType{
            true,
            R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
            type::UrlResponse{{},
                              {},
                              net::HttpStatusCode::HTTP_SERVICE_UNAVAILABLE,
                              {},
                              {}},
            {type::Result::LEDGER_ERROR, {}}},
        // "invalid_body_in_balance_server_response"
        GetAnonFundsTestParamType{
            true,
            R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
            type::UrlResponse{{}, {}, net::HttpStatusCode::HTTP_OK, {}, {}},
            {type::Result::LEDGER_ERROR, 0.0}},
        // "happy_path"
        GetAnonFundsTestParamType{
            true,
            R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
            type::UrlResponse{
                {},
                {},
                net::HttpStatusCode::HTTP_OK,
                R"({ "total": 5.0, "spendable": 0.0, "confirmed": 5.0, "unconfirmed": 0.0 })",
                {}},
            {type::Result::LEDGER_OK, 5.0}}),
    GetAnonFundsNameSuffixGenerator);

TEST_P(GetAnonFundsTest, AnonFundsFlow) {
  const auto& params = GetParam();
  const bool fetch_old_balance_enabled = std::get<0>(params);
  const std::string& brave_wallet = std::get<1>(params);
  const type::UrlResponse& balance_server_response = std::get<2>(params);
  const auto& expected = std::get<3>(params);

  ON_CALL(*mock_ledger_client_, GetBooleanState(state::kFetchOldBalance))
      .WillByDefault(testing::Return(fetch_old_balance_enabled));

  ON_CALL(*mock_ledger_client_, GetEncryptedStringState(state::kWalletBrave))
      .WillByDefault(testing::Return(brave_wallet));

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(testing::Return(mock_database_.get()));

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
            callback(balance_server_response);
          }));

  authorization_->GetAnonFunds(
      [&](const type::Result result, type::BalancePtr balance) {
        const auto expected_result = std::get<0>(expected);
        EXPECT_TRUE(result == expected_result);

        const auto& expected_balance = std::get<1>(expected);
        if (!balance) {
          EXPECT_FALSE(expected_balance);
        } else {
          EXPECT_TRUE(expected_balance);
          EXPECT_DOUBLE_EQ(balance->user_funds, *expected_balance);
        }
      });
}

using LinkWalletTestParamType =
    std::tuple<std::string,        // Uphold wallet
               type::UrlResponse,  // rewards web services response
               type::Result        // expected result
               >;

class LinkWalletTest : public TestWithParam<LinkWalletTestParamType> {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<UpholdAuthorization> authorization_;

 public:
  LinkWalletTest()
      : mock_ledger_client_{std::make_unique<MockLedgerClient>()},
        mock_ledger_impl_{
            std::make_unique<MockLedgerImpl>(mock_ledger_client_.get())},
        authorization_{
            std::make_unique<UpholdAuthorization>(mock_ledger_impl_.get())} {}
};

std::string LinkWalletNameSuffixGenerator(
    const TestParamInfo<LinkWalletTest::ParamType>& info) {
  const std::string& uphold_wallet = std::get<0>(info.param);
  const type::UrlResponse& rewards_web_services_response =
      std::get<1>(info.param);

  if (uphold_wallet.empty()) {
    return "UpholdWalletIsNull";
  }

  if (rewards_web_services_response.status_code ==
      net::HttpStatusCode::HTTP_NOT_FOUND) {
    return "404";
  }

  if (rewards_web_services_response.status_code ==
      net::HttpStatusCode::HTTP_CONFLICT) {
    return "WalletDeviceLimitReached";
  }

  if (rewards_web_services_response.status_code !=
      net::HttpStatusCode::HTTP_OK) {
    return "RewardsWebServicesError";
  }

  return "HappyPath";
}

const char uphold_wallet[]{R"(
{
    "account_url": "https://wallet-sandbox.uphold.com/dashboard",
    "add_url": "",
    "address": "962df5b1-bb72-4619-a349-c8087941b795",
    "fees": {},
    "login_url": "https://wallet-sandbox.uphold.com/authorize/...",
    "one_time_string": "...",
    "status": 2,
    "token": "0047c2fd8f023e067354dbdb5639ee67acf77150",
    "user_name": "",
    "verify_url": "https://wallet-sandbox.uphold.com/authorize/...",
    "withdraw_url": ""
}
)"};

INSTANTIATE_TEST_SUITE_P(
    UpholdAuthorization,
    LinkWalletTest,
    Values(
        // "uphold_wallet_is_null"
        LinkWalletTestParamType{"", type::UrlResponse{},
                                type::Result::LEDGER_ERROR},
        // "404"
        LinkWalletTestParamType{
            uphold_wallet,
            type::UrlResponse{{},
                              {},
                              net::HttpStatusCode::HTTP_NOT_FOUND,
                              {},
                              {}},
            type::Result::NOT_FOUND},
        // "wallet_device_limit_reached"
        LinkWalletTestParamType{
            uphold_wallet,
            type::UrlResponse{{},
                              {},
                              net::HttpStatusCode::HTTP_CONFLICT,
                              {},
                              {}},
            type::Result::ALREADY_EXISTS},
        // "rewards_web_services_error"
        LinkWalletTestParamType{
            uphold_wallet,
            type::UrlResponse{{},
                              {},
                              net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
                              {},
                              {}},
            type::Result::LEDGER_ERROR},
        // "happy_path"
        LinkWalletTestParamType{
            uphold_wallet,
            type::UrlResponse{{}, {}, net::HttpStatusCode::HTTP_OK, {}, {}},
            type::Result::LEDGER_OK}),
    LinkWalletNameSuffixGenerator);

TEST_P(LinkWalletTest, LinkWalletFlow) {
  const auto& params = GetParam();
  const std::string& uphold_wallet = std::get<0>(params);
  const type::UrlResponse& rewards_web_services_response = std::get<1>(params);
  const auto& expected_result = std::get<2>(params);

  ON_CALL(*mock_ledger_client_, GetEncryptedStringState(state::kWalletUphold))
      .WillByDefault(testing::Return(uphold_wallet));

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
            callback(rewards_web_services_response);
          }));

  authorization_->LinkWallet(0.0, [&](const type::Result result) {
    EXPECT_TRUE(result == expected_result);
  });
}

}  // namespace uphold
}  // namespace ledger
