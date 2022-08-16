/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <tuple>
#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/promotion/promotion_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UpholdTest*

using ::testing::_;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::Exactly;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;
using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace ledger {
namespace uphold {

class UpholdTest : public Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<database::MockDatabase> mock_database_;
  std::unique_ptr<promotion::MockPromotion> mock_promotion_;
  std::unique_ptr<Uphold> uphold_;

  UpholdTest()
      : mock_ledger_client_{std::make_unique<MockLedgerClient>()},
        mock_ledger_impl_{
            std::make_unique<MockLedgerImpl>(mock_ledger_client_.get())},
        mock_database_{
            std::make_unique<database::MockDatabase>(mock_ledger_impl_.get())},
        mock_promotion_{std::make_unique<promotion::MockPromotion>(
            mock_ledger_impl_.get())},
        uphold_{std::make_unique<Uphold>(mock_ledger_impl_.get())} {}
};

TEST_F(UpholdTest, FetchBalanceConnectedWallet) {
  const std::string wallet = FakeEncryption::Base64EncryptString(R"({
      "token":"token",
      "address":"address"
      "status":1
    })");
  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(Return(wallet));
  EXPECT_CALL(*mock_ledger_client_, LoadURL(_, _)).Times(0);

  FetchBalanceCallback callback =
      base::BindOnce([](type::Result result, double balance) {
        ASSERT_EQ(result, type::Result::LEDGER_OK);
        ASSERT_EQ(balance, 0.0);
      });

  uphold_->FetchBalance(std::move(callback));
}

absl::optional<type::WalletStatus> GetStatusFromJSON(
    const std::string& uphold_wallet) {
  auto value = base::JSONReader::Read(uphold_wallet);
  if (value && value->is_dict()) {
    base::DictionaryValue* dictionary = nullptr;
    if (value->GetAsDictionary(&dictionary)) {
      if (auto status = dictionary->FindIntKey("status")) {
        return static_cast<type::WalletStatus>(*status);
      }
    }
  }

  return {};
}

template <typename ParamType>
std::string NameSuffixGenerator(const TestParamInfo<ParamType>& info) {
  return std::get<0>(info.param);
}

// clang-format off
using AuthorizeParamType = std::tuple<
    std::string,                               // test name suffix
    std::string,                               // input Uphold wallet
    base::flat_map<std::string, std::string>,  // input args
    type::UrlResponse,                         // Uphold OAuth response
    type::Result,                              // expected result
    base::flat_map<std::string, std::string>,  // expected args
    absl::optional<type::WalletStatus>         // expected status
>;

struct Authorize : UpholdTest,
                   WithParamInterface<AuthorizeParamType> {};

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  Authorize,
  Values(
    AuthorizeParamType{  // Uphold wallet is null!
      "00_uphold_wallet_is_null",
      {},
      {},
      {},
      type::Result::LEDGER_ERROR,
      {},
      {}
    },
    AuthorizeParamType{  // Attempting to re-authorize in VERIFIED status!
      "01_VERIFIED_attempting_to_re_authorize",
      R"({ "status": 2 })",
      {},
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::VERIFIED
    },
    // NOLINTNEXTLINE
    AuthorizeParamType{  // Uphold returned with an error - the user is not KYC'd. (NOT_CONNECTED)
      // NOLINTNEXTLINE
      "02_NOT_CONNECTED_uphold_returned_with_user_does_not_meet_minimum_requirements",
      R"({ "status": 0 })",
      { { "error_description", "User does not meet minimum requirements" } },
      {},
      type::Result::NOT_FOUND,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    AuthorizeParamType{  // Uphold returned with an error - user's region is not supported. (NOT_CONNECTED)
      // NOLINTNEXTLINE
      "03_NOT_CONNECTED_uphold_returned_with_application_not_available_for_user_geolocation",
      R"({ "status": 0 })",
      { { "error_description", "Application not available for user geolocation" } },  // NOLINT
      {},
      type::Result::REGION_NOT_SUPPORTED,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    AuthorizeParamType{  // Uphold returned with an error - theoretically not possible. (NOT_CONNECTED)
      "04_NOT_CONNECTED_uphold_returned_with_an_error",
      R"({ "status": 0 })",
      { { "error_description", "some other reason" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    AuthorizeParamType{  // Arguments are empty! (NOT_CONNECTED)
      "05_NOT_CONNECTED_arguments_are_empty",
      R"({ "status": 0 })",
      {},
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    AuthorizeParamType{  // code is empty! (NOT_CONNECTED)
      "06_NOT_CONNECTED_code_is_empty",
      R"({ "status": 0 })",
      { { "code", "" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    AuthorizeParamType{  // state is empty! (NOT_CONNECTED)
      "07_NOT_CONNECTED_state_is_empty",
      R"({ "status": 0 })",
      { { "code", "code" }, { "state", "" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    AuthorizeParamType{  // One-time string mismatch! (NOT_CONNECTED)
      "08_NOT_CONNECTED_one_time_string_mismatch",
      R"({ "status": 0, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "mismatch" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    AuthorizeParamType{  // Couldn't exchange code for the access token! (NOT_CONNECTED)
      "09_NOT_CONNECTED_couldn_t_exchange_code_for_the_access_token",
      R"({ "status": 0, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "one_time_string" } },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    AuthorizeParamType{  // Access token is empty! (NOT_CONNECTED)
      "10_NOT_CONNECTED_access_token_is_empty",
      R"({ "status": 0, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "one_time_string" } },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "access_token": "" })",
        {}
      },
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::NOT_CONNECTED
    },
    AuthorizeParamType{  // Happy path. (NOT_CONNECTED)
      "11_NOT_CONNECTED_happy_path",
      R"({ "status": 0, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "one_time_string" } },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "access_token": "access_token" })",
        {}
      },
      type::Result::LEDGER_OK,
      {},
      type::WalletStatus::PENDING
    },
    // NOLINTNEXTLINE
    AuthorizeParamType{  // Uphold returned with an error - the user is not KYC'd. (DISCONNECTED_VERIFIED)
      // NOLINTNEXTLINE
      "12_DISCONNECTED_VERIFIED_uphold_returned_with_user_does_not_meet_minimum_requirements",
      R"({ "status": 4 })",
      { { "error_description", "User does not meet minimum requirements" } },
      {},
      type::Result::NOT_FOUND,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    // NOLINTNEXTLINE
    AuthorizeParamType{  // Uphold returned with an error - theoretically not possible. (DISCONNECTED_VERIFIED)
      "13_DISCONNECTED_VERIFIED_uphold_returned_with_an_error",
      R"({ "status": 4 })",
      { { "error_description", "some other reason" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    AuthorizeParamType{  // Arguments are empty! (DISCONNECTED_VERIFIED)
      "14_DISCONNECTED_VERIFIED_arguments_are_empty",
      R"({ "status": 4 })",
      {},
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    AuthorizeParamType{  // code is empty! (DISCONNECTED_VERIFIED)
      "15_DISCONNECTED_VERIFIED_code_is_empty",
      R"({ "status": 4 })",
      { { "code", "" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    AuthorizeParamType{  // state is empty! (DISCONNECTED_VERIFIED)
      "16_DISCONNECTED_VERIFIED_state_is_empty",
      R"({ "status": 4 })",
      { { "code", "code" }, { "state", "" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    AuthorizeParamType{  // One-time string mismatch! (DISCONNECTED_VERIFIED)
      "17_DISCONNECTED_VERIFIED_one_time_string_mismatch",
      R"({ "status": 4, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "mismatch" } },
      {},
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    // NOLINTNEXTLINE
    AuthorizeParamType{  // Couldn't exchange code for the access token! (DISCONNECTED_VERIFIED)
      "18_DISCONNECTED_VERIFIED_couldn_t_exchange_code_for_the_access_token",
      R"({ "status": 4, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "one_time_string" } },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    AuthorizeParamType{  // Access token is empty! (DISCONNECTED_VERIFIED)
      "19_DISCONNECTED_VERIFIED_access_token_is_empty",
      R"({ "status": 4, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "one_time_string" } },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "access_token": "" })",
        {}
      },
      type::Result::LEDGER_ERROR,
      {},
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    AuthorizeParamType{  // Happy path. (DISCONNECTED_VERIFIED)
      "20_DISCONNECTED_VERIFIED_happy_path",
      R"({ "status": 4, "one_time_string": "one_time_string" })",
      { { "code", "code" }, { "state", "one_time_string" } },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "access_token": "access_token" })",
        {}
      },
      type::Result::LEDGER_OK,
      {},
      type::WalletStatus::PENDING
    }),
  NameSuffixGenerator<AuthorizeParamType>
);
// clang-format on

TEST_P(Authorize, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& input_args = std::get<2>(params);
  const auto& uphold_oauth_response = std::get<3>(params);
  const auto expected_result = std::get<4>(params);
  const auto& expected_args = std::get<5>(params);
  const auto expected_status = std::get<6>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
            std::move(callback).Run(uphold_oauth_response);
          });

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  uphold_->WalletAuthorization(
      input_args,
      [&](type::Result result, base::flat_map<std::string, std::string> args) {
        ASSERT_EQ(result, expected_result);
        ASSERT_EQ(args, expected_args);

        const auto status = GetStatusFromJSON(uphold_wallet);
        if (status && expected_status) {
          ASSERT_EQ(*status, *expected_status);
        } else {
          ASSERT_TRUE(!status && !expected_status);
        }
      });
}

// clang-format off
using GenerateParamType = std::tuple<
    std::string,                         // test name suffix
    std::string,                         // input Uphold wallet
    type::Result,                        // expected result
    absl::optional<type::WalletStatus>,  // expected status
    bool                                 // expected to call TransferTokens
>;

struct Generate : UpholdTest,
                  WithParamInterface<GenerateParamType> {};

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  Generate,
  Values(
    GenerateParamType{  // Happy path (no wallet).
      "00_happy_path_no_wallet",
      {},
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED,
      false
    },
    GenerateParamType{  // Happy path (NOT_CONNECTED).
      "01_happy_path_NOT_CONNECTED",
      R"({ "status": 0 })",
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED,
      false
    },
    GenerateParamType{  // Happy path (DISCONNECTED_VERIFIED).
      "02_happy_path_DISCONNECTED_VERIFIED",
      R"({ "status": 4 })",
      type::Result::LEDGER_OK,
      type::WalletStatus::DISCONNECTED_VERIFIED,
      true
    }),
  NameSuffixGenerator<GenerateParamType>
);
// clang-format on

TEST_P(Generate, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto expected_result = std::get<2>(params);
  const auto expected_status = std::get<3>(params);
  const bool expected_to_call_transfer_tokens = std::get<4>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  ON_CALL(*mock_ledger_impl_, promotion())
      .WillByDefault(Return(mock_promotion_.get()));

  EXPECT_CALL(*mock_promotion_, TransferTokens(_))
      .Times(expected_to_call_transfer_tokens ? 1 : 0);

  uphold_->GenerateWallet(base::BindLambdaForTesting([&](type::Result result) {
    ASSERT_EQ(result, expected_result);

    const auto status = GetStatusFromJSON(uphold_wallet);
    if (status && expected_status) {
      ASSERT_EQ(*status, *expected_status);
    } else {
      ASSERT_TRUE(!status && !expected_status);
    }
  }));
}

// clang-format off
using GetUserParamType = std::tuple<
    std::string,                        // test name suffix
    std::string,                        // input Uphold wallet
    type::UrlResponse,                  // Uphold Get User response
    type::Result,                       // expected result
    absl::optional<type::WalletStatus>  // expected status
>;

struct GetUser : UpholdTest,
                 WithParamInterface<GetUserParamType> {};

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  GetUser,
  Values(
    GetUserParamType{  // Access token expired! (PENDING)
      "00_PENDING_access_token_expired",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_UNAUTHORIZED,
        {},
        {}
      },
      type::Result::EXPIRED_TOKEN,
      type::WalletStatus::NOT_CONNECTED
    },
    GetUserParamType{  // Couldn't get the user object from Uphold! (PENDING)
      "01_PENDING_couldn_t_get_the_user_object_from_uphold",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    GetUserParamType{  // BAT is not allowed for the user! (PENDING)
      "02_PENDING_bat_is_not_allowed_for_the_user",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [] })",
        {}
      },
      type::Result::UPHOLD_BAT_NOT_ALLOWED,
      type::WalletStatus::NOT_CONNECTED
    },
    GetUserParamType{  // Access token expired! (VERIFIED)
      "03_VERIFIED_access_token_expired",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_UNAUTHORIZED,
        {},
        {}
      },
      type::Result::EXPIRED_TOKEN,
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    GetUserParamType{  // Couldn't get the user object from Uphold! (VERIFIED)
      "04_VERIFIED_couldn_t_get_the_user_object_from_uphold",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::CONTINUE,
      type::WalletStatus::VERIFIED
    },
    GetUserParamType{  // BAT is not allowed for the user! (VERIFIED)
      "05_VERIFIED_bat_is_not_allowed_for_the_user",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [] })",
        {}
      },
      type::Result::UPHOLD_BAT_NOT_ALLOWED,
      type::WalletStatus::DISCONNECTED_VERIFIED
    }),
  NameSuffixGenerator<GetUserParamType>
);
// clang-format on

TEST_P(GetUser, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& uphold_get_user_response = std::get<2>(params);
  const auto expected_result = std::get<3>(params);
  const auto expected_status = std::get<4>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
            std::move(callback).Run(uphold_get_user_response);
          });

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  uphold_->GenerateWallet(base::BindLambdaForTesting(
      [&](type::Result result) { ASSERT_EQ(result, expected_result); }));

  const auto status = GetStatusFromJSON(uphold_wallet);
  if (status && expected_status) {
    ASSERT_EQ(*status, *expected_status);
  } else {
    ASSERT_TRUE(!status && !expected_status);
  }
}

// clang-format off
using GetCapabilitiesParamType = std::tuple<
    std::string,                        // test name suffix
    std::string,                        // input Uphold wallet
    type::UrlResponse,                  // Uphold Get User response
    type::UrlResponse,                  // Uphold Get Capabilities response
    type::UrlResponse,                  // Rewards Delete Claim response
    type::Result,                       // expected result
    absl::optional<type::WalletStatus>  // expected status
>;

struct GetCapabilities : UpholdTest,
                         WithParamInterface<GetCapabilitiesParamType> {};

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  GetCapabilities,
  Values(
    GetCapabilitiesParamType{  // Access token expired! (PENDING)
      "00_PENDING_access_token_expired",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_UNAUTHORIZED,
        {},
        {}
      },
      type::UrlResponse{},
      type::Result::EXPIRED_TOKEN,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (PENDING)
      "01_PENDING_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (PENDING)
      "02_PENDING_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (PENDING)
      "03_PENDING_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (PENDING)
      "04_PENDING_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true } ])",
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // User doesn't have the required Uphold capabilities! (PENDING)
      "05_PENDING_user_doesnt_have_the_required_uphold_capabilities",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": false, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{},
      type::Result::UPHOLD_INSUFFICIENT_CAPABILITIES,
      type::WalletStatus::NOT_CONNECTED
    },
    GetCapabilitiesParamType{  // Access token expired! (VERIFIED)
      "06_VERIFIED_access_token_expired",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_UNAUTHORIZED,
        {},
        {}
      },
      type::UrlResponse{},
      type::Result::EXPIRED_TOKEN,
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (VERIFIED)
      "07_VERIFIED_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::VERIFIED
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (VERIFIED)
      "08_VERIFIED_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::VERIFIED
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (VERIFIED)
      "09_VERIFIED_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::VERIFIED
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // Couldn't get capabilities from Uphold! (VERIFIED)
      "10_VERIFIED_couldn_t_get_capabilities_from_uphold",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true } ])",
        {}
      },
      type::UrlResponse{},
      type::Result::CONTINUE,
      type::WalletStatus::VERIFIED
    },
    // NOLINTNEXTLINE
    GetCapabilitiesParamType{  // User doesn't have the required Uphold capabilities! (VERIFIED)
      "11_VERIFIED_user_doesnt_have_the_required_uphold_capabilities",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": false, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::UPHOLD_INSUFFICIENT_CAPABILITIES,
      type::WalletStatus::NOT_CONNECTED
    },
    GetCapabilitiesParamType{  // Happy path. (VERIFIED)
      "12_VERIFIED_happy_path",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{},
      type::Result::LEDGER_OK,
      type::WalletStatus::VERIFIED
    }),
  NameSuffixGenerator<GetCapabilitiesParamType>
);
// clang-format on

TEST_P(GetCapabilities, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& uphold_get_user_response = std::get<2>(params);
  const auto& uphold_get_capabilities_response = std::get<3>(params);
  const auto& rewards_services_delete_claim_response = std::get<4>(params);
  const auto expected_result = std::get<5>(params);
  const auto expected_status = std::get<6>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  EXPECT_CALL(*mock_ledger_client_, LoadURL(_, _))
      .Times(AtMost(3))
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_user_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_capabilities_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(rewards_services_delete_claim_response);
      });

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  ON_CALL(*mock_ledger_impl_, promotion())
      .WillByDefault(Return(mock_promotion_.get()));

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
      .WillByDefault([] {
        return R"(
          {
            "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db",
            "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w="
          }
        )";
      });

  uphold_->GenerateWallet(base::BindLambdaForTesting(
      [&](type::Result result) { ASSERT_EQ(result, expected_result); }));

  const auto status = GetStatusFromJSON(uphold_wallet);
  if (status && expected_status) {
    ASSERT_EQ(*status, *expected_status);
  } else {
    ASSERT_TRUE(!status && !expected_status);
  }
}

// clang-format off
using GetCardIDParamType = std::tuple<
    std::string,                        // test name suffix
    std::string,                        // input Uphold wallet
    type::UrlResponse,                  // Uphold Get User response
    type::UrlResponse,                  // Uphold Get Capabilities response
    type::UrlResponse,                  // Uphold List Cards response
    type::UrlResponse,                  // Uphold Create Card response
    type::UrlResponse,                  // Uphold Update Card response
    type::Result,                       // expected result
    absl::optional<type::WalletStatus>  // expected status
>;

struct GetCardID : UpholdTest,
                   WithParamInterface<GetCardIDParamType> {};

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  GetCardID,
  Values(
    GetCardIDParamType{  // Access token expired! (List Cards)
      "00_list_cards_access_token_expired",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_UNAUTHORIZED,
        {},
        {}
      },
      {},
      {},
      type::Result::EXPIRED_TOKEN,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    GetCardIDParamType{  // List Cards failed && Access token expired! (Create Card)
      "01_create_card_access_token_expired",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_UNAUTHORIZED,
        {},
        {}
      },
      {},
      type::Result::EXPIRED_TOKEN,
      type::WalletStatus::NOT_CONNECTED
    },
    GetCardIDParamType{  // Create Card failed.
      "02_create_card_failed",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true }, { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      {},
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    GetCardIDParamType{  // Create Card succeeded && id is empty.
      "03_create_card_succeeded_but_id_is_empty",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true }, { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "id": "" })",
        {}
      },
      {},
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    // NOLINTNEXTLINE
    GetCardIDParamType{  // Create Card succeeded && Access token expired! (Update Card)
      "04_update_card_access_token_expired",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "id": "962ef3b8-bc12-4619-a349-c8083931b795" })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_UNAUTHORIZED,
        {},
        {}
      },
      type::Result::EXPIRED_TOKEN,
      type::WalletStatus::NOT_CONNECTED
    },
    GetCardIDParamType{  // Update Card failed.
      "05_update_card_failed",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true }, { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "id": "962ef3b8-bc12-4619-a349-c8083931b795" })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    }),
  NameSuffixGenerator<GetCardIDParamType>
);
// clang-format on

TEST_P(GetCardID, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& uphold_get_user_response = std::get<2>(params);
  const auto& uphold_get_capabilities_response = std::get<3>(params);
  const auto& uphold_list_cards_response = std::get<4>(params);
  const auto& uphold_create_card_response = std::get<5>(params);
  const auto& uphold_update_card_response = std::get<6>(params);
  const auto expected_result = std::get<7>(params);
  const auto expected_status = std::get<8>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  EXPECT_CALL(*mock_ledger_client_, LoadURL(_, _))
      .Times(AtMost(5))
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_user_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_capabilities_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_list_cards_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_create_card_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_update_card_response);
      });

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  uphold_->GenerateWallet(base::BindLambdaForTesting([&](type::Result result) {
    ASSERT_EQ(result, expected_result);

    const auto status = GetStatusFromJSON(uphold_wallet);
    if (status && expected_status) {
      ASSERT_EQ(*status, *expected_status);
    } else {
      ASSERT_TRUE(!status && !expected_status);
    }
  }));
}

// clang-format off
using GetAnonFundsParamType = std::tuple<
    std::string,                        // test name suffix
    std::string,                        // input Uphold wallet
    type::UrlResponse,                  // Uphold Get User response
    type::UrlResponse,                  // Uphold Get Capabilities response
    type::UrlResponse,                  // Uphold List Cards response
    bool,                               // fetch_old_balance
    std::string,                        // input Rewards wallet
    type::UrlResponse,                  // Rewards Get Wallet Balance response
    type::Result,                       // expected result
    absl::optional<type::WalletStatus>  // expected status
>;

struct GetAnonFunds : UpholdTest,
                      WithParamInterface<GetAnonFundsParamType> {};

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  GetAnonFunds,
  Values(
    GetAnonFundsParamType{  // Payment ID is empty!
      "00_payment_id_is_empty",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true }, { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      true,
      R"({ "payment_id": "", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      {},
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    GetAnonFundsParamType{  // Rewards Get Wallet Balance failed.
      "01_get_wallet_balance_failed",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true }, { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    }),
  NameSuffixGenerator<GetAnonFundsParamType>
);
// clang-format on

TEST_P(GetAnonFunds, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& uphold_get_user_response = std::get<2>(params);
  const auto& uphold_get_capabilities_response = std::get<3>(params);
  const auto& uphold_list_cards_response = std::get<4>(params);
  const auto fetch_old_balance = std::get<5>(params);
  const auto& input_rewards_wallet = std::get<6>(params);
  const auto& rewards_services_get_wallet_balance_response =
      std::get<7>(params);
  const auto expected_result = std::get<8>(params);
  const auto expected_status = std::get<9>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  EXPECT_CALL(*mock_ledger_client_, LoadURL(_, _))
      .Times(AtMost(4))
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_user_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_capabilities_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_list_cards_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(rewards_services_get_wallet_balance_response);
      });

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  ON_CALL(*mock_ledger_client_, GetBooleanState(state::kFetchOldBalance))
      .WillByDefault(Return(fetch_old_balance));

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
      .WillByDefault(Return(input_rewards_wallet));

  uphold_->GenerateWallet(base::BindLambdaForTesting([&](type::Result result) {
    ASSERT_EQ(result, expected_result);

    const auto status = GetStatusFromJSON(uphold_wallet);
    if (status && expected_status) {
      ASSERT_EQ(*status, *expected_status);
    } else {
      ASSERT_TRUE(!status && !expected_status);
    }
  }));
}

// clang-format off
using LinkWalletParamType = std::tuple<
    std::string,                        // test name suffix
    std::string,                        // input Uphold wallet
    type::UrlResponse,                  // Uphold Get User response
    type::UrlResponse,                  // Uphold Get Capabilities response
    type::UrlResponse,                  // Uphold List Cards response
    bool,                               // fetch_old_balance
    std::string,                        // input Rewards wallet
    type::UrlResponse,                  // Rewards Link (Claim) Wallet response
    type::Result,                       // expected result
    absl::optional<type::WalletStatus>  // expected status
>;

struct LinkWallet : UpholdTest,
                    WithParamInterface<LinkWalletParamType> {};

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  LinkWallet,
  Values(
    LinkWalletParamType{  // Device limit reached.
      "00_device_limit_reached",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_CONFLICT,
        {},
        {}
      },
      type::Result::DEVICE_LIMIT_REACHED,
      type::WalletStatus::NOT_CONNECTED
    },
    LinkWalletParamType{  // KYC required.
      "01_kyc_required",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_FORBIDDEN,
        R"(
          {
              "message": "error linking wallet: KYC required: user kyc did not pass",
              "code": 403
          }
        )",
        {}
      },
      type::Result::NOT_FOUND,
      type::WalletStatus::NOT_CONNECTED
    },
    LinkWalletParamType{  // Mismatched provider accounts.
      "02_mismatched_provider_accounts",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_FORBIDDEN,
        R"(
          {
              "message": "error linking wallet: unable to link wallets: mismatched provider accounts: wallets do not match",
              "code": 403
          }
        )",
        {}
      },
      type::Result::MISMATCHED_PROVIDER_ACCOUNTS,
      type::WalletStatus::NOT_CONNECTED
    },
    LinkWalletParamType{  // Transaction verification failure.
      "03_transaction_verification_failure",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_FORBIDDEN,
        R"(
          {
              "message": "error linking wallet: transaction verification failure: failed to verify transaction",
              "code": 403
          }
        )",
        {}
      },
      type::Result::UPHOLD_TRANSACTION_VERIFICATION_FAILURE,
      type::WalletStatus::NOT_CONNECTED
    },
    LinkWalletParamType{  // Flagged wallet.
      "04_flagged_wallet",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_BAD_REQUEST,
        R"(
          {
              "message": "unable to link - unusual activity",
              "code": 400
          }
        )",
        {}
      },
      type::Result::FLAGGED_WALLET,
      type::WalletStatus::NOT_CONNECTED
    },
    LinkWalletParamType{  // Region not supported.
      "05_region_not_supported",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_BAD_REQUEST,
        R"(
          {
              "message": "region not supported: failed to validate account: invalid country",
              "code": 400
          }
        )",
        {}
      },
      type::Result::REGION_NOT_SUPPORTED,
      type::WalletStatus::NOT_CONNECTED
    },
    LinkWalletParamType{  // Mismatched provider account regions.
      "06_mismatched_provider_account_regions",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_BAD_REQUEST,
        R"(
          {
              "message": "error linking wallet: mismatched provider account regions: geo reset is different",
              "code": 400
          }
        )",
        {}
      },
      type::Result::MISMATCHED_PROVIDER_ACCOUNT_REGIONS,
      type::WalletStatus::NOT_CONNECTED
    },
    LinkWalletParamType{  // Rewards Link (Claim) Wallet failed.
      "07_link_wallet_failed",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true }, { "key": "sends", "enabled": true } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::CONTINUE,
      type::WalletStatus::PENDING
    },
    LinkWalletParamType{  // Happy path.
      "08_happy_path",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"({ "currencies": [ "BAT" ] })",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "key": "receives", "enabled": true, "requirements": [] }, { "key": "sends", "enabled": true, "requirements": [] } ])",
        {}
      },
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"([ { "id": "962ef3b8-bc12-4619-a349-c8083931b795", "label": "Brave Browser" } ])",
        {}
      },
      false,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::VERIFIED
    }),
  NameSuffixGenerator<LinkWalletParamType>
);
// clang-format on

TEST_P(LinkWallet, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& uphold_get_user_response = std::get<2>(params);
  const auto& uphold_get_capabilities_response = std::get<3>(params);
  const auto& uphold_list_cards_response = std::get<4>(params);
  const auto fetch_old_balance = std::get<5>(params);
  const auto& input_rewards_wallet = std::get<6>(params);
  const auto& rewards_link_wallet_response = std::get<7>(params);
  const auto expected_result = std::get<8>(params);
  const auto expected_status = std::get<9>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  EXPECT_CALL(*mock_ledger_client_, LoadURL(_, _))
      .Times(AtMost(4))
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_user_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_get_capabilities_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(uphold_list_cards_response);
      })
      .WillOnce([&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        std::move(callback).Run(rewards_link_wallet_response);
      });

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  ON_CALL(*mock_ledger_impl_, promotion())
      .WillByDefault(Return(mock_promotion_.get()));

  ON_CALL(*mock_ledger_client_, GetBooleanState(state::kFetchOldBalance))
      .WillByDefault(Return(fetch_old_balance));

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
      .WillByDefault(Return(input_rewards_wallet));

  uphold_->GenerateWallet(base::BindLambdaForTesting(
      [&](type::Result result) { ASSERT_EQ(result, expected_result); }));

  const auto status = GetStatusFromJSON(uphold_wallet);
  if (status && expected_status) {
    ASSERT_EQ(*status, *expected_status);
  } else {
    ASSERT_TRUE(!status && !expected_status);
  }
}

// clang-format off
using DisconnectWalletParamType = std::tuple<
    std::string,                        // test name suffix
    std::string,                        // input Uphold wallet
    std::string,                        // input Rewards wallet
    // NOLINTNEXTLINE
    type::UrlResponse,                  // Rewards UnLink (Claim) Wallet response
    type::Result,                       // expected result
    absl::optional<type::WalletStatus>  // expected status
>;

struct DisconnectUpholdWallet : UpholdTest,
                                WithParamInterface<DisconnectWalletParamType> {};  // NOLINT

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  DisconnectUpholdWallet,
  Values(
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (NOT_CONNECTED)
      "00_NOT_CONNECTED_unlink_wallet_succeeded",
      R"({ "status": 0 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (NOT_CONNECTED)
      "01_NOT_CONNECTED_unlink_wallet_failed",
      R"({ "status": 0 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (VERIFIED)
      "02_VERIFIED_unlink_wallet_succeeded",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (VERIFIED)
      "03_VERIFIED_unlink_wallet_failed",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      type::WalletStatus::VERIFIED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (DISCONNECTED_VERIFIED)
      "04_DISCONNECTED_VERIFIED_unlink_wallet_succeeded",
      R"({ "status": 4 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (DISCONNECTED_VERIFIED)
      "05_DISCONNECTED_VERIFIED_unlink_wallet_failed",
      R"({ "status": 4 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      type::WalletStatus::DISCONNECTED_VERIFIED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (PENDING)
      "06_PENDING_unlink_wallet_succeeded",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (PENDING)
      "07_PENDING_unlink_wallet_failed",
      R"({ "status": 5, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    }),
  NameSuffixGenerator<DisconnectWalletParamType>
);
// clang-format on

TEST_P(DisconnectUpholdWallet, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& input_rewards_wallet = std::get<2>(params);
  const auto& rewards_unlink_wallet_response = std::get<3>(params);
  const auto expected_result = std::get<4>(params);
  const auto expected_status = std::get<5>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
      .WillByDefault(Return(input_rewards_wallet));

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
            std::move(callback).Run(rewards_unlink_wallet_response);
          });

  mock_ledger_impl_->SetInitializedForTesting();

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  mock_ledger_impl_->DisconnectWallet(
      constant::kWalletUphold, [&](type::Result result) {
        ASSERT_EQ(result, expected_result);

        const auto status = GetStatusFromJSON(uphold_wallet);
        if (status && expected_status) {
          ASSERT_EQ(*status, *expected_status);
        } else {
          ASSERT_TRUE(!status && !expected_status);
        }
      });
}

}  // namespace uphold
}  // namespace ledger
