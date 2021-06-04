/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/task_environment.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_authorization.h"
#include "bat/ledger/option_keys.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=UpholdAuthorizationTest*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestWithParam;
using ::testing::TestParamInfo;
using ::testing::Values;

namespace ledger {
namespace uphold {
namespace tests {
namespace anon_funds {
namespace get {

using Result = std::pair<type::Result, base::Optional<double>>;
using ParamType =
  std::tuple<
    bool,              // contributions disabled for BAP migration
    bool,              // fetch old balance enabled
    std::string,       // Brave wallet
    type::UrlResponse, // balance server response
    Result             // expected result
  >
;

class Get : public TestWithParam<ParamType> {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<database::MockDatabase> mock_database_;
  std::unique_ptr<UpholdAuthorization> authorization_;

 public:
  Get()
    : mock_ledger_client_{ std::make_unique<MockLedgerClient>() }
    , mock_ledger_impl_{ std::make_unique<MockLedgerImpl>(mock_ledger_client_.get()) }
    , mock_database_{ std::make_unique<database::MockDatabase>(mock_ledger_impl_.get()) }
    , authorization_{ std::make_unique<UpholdAuthorization>(mock_ledger_impl_.get()) }
  {}
};

std::string NameSuffixGenerator(const TestParamInfo<Get::ParamType>& info) {
  const bool contributions_disabled_for_BAP_migration = std::get<0>(info.param);
  const bool fetch_old_balance_enabled = std::get<1>(info.param);
  const std::string& brave_wallet = std::get<2>(info.param);
  const type::UrlResponse& balance_server_response = std::get<3>(info.param);

  if (contributions_disabled_for_BAP_migration) {
    return "contributions_disabled_for_BAP_migration";
  }

  if (!fetch_old_balance_enabled) {
    return "fetch_old_balance_disabled";
  }

  if (brave_wallet.empty()) {
    return "brave_wallet_is_not_created";
  }

  if (brave_wallet.find(R"("payment_id": "")") != std::string::npos) {
    return "brave_wallet_payment_id_is_empty";
  }
  
  if (balance_server_response.status_code != net::HttpStatusCode::HTTP_OK) {
    return "balance_server_error";
  }
  
  if (balance_server_response.body.empty()) {
    return "invalid_body_in_balance_server_response";
  }

  return "happy_path";
}

INSTANTIATE_TEST_SUITE_P(
  UpholdAuthorizationTest,
  Get,
  Values(
    // "contributions_disabled_for_BAP_migration"
    ParamType{
      true,
      {},
      {},
      {},
      { type::Result::LEDGER_OK, 0.0 }
    },
    // "fetch_old_balance_disabled"
    ParamType{
      false,
      false,
      {},
      {},
      { type::Result::LEDGER_OK, 0.0 }
    },
    // "brave_wallet_is_not_created"
    ParamType{
      false,
      true,
      {},
      {},
      { type::Result::LEDGER_OK, 0.0 }
    },
    // "brave_wallet_payment_id_is_empty"
    ParamType{
      false,
      true,
      R"({ "payment_id": "", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      {},
      { type::Result::LEDGER_ERROR, {} }
    },
    // "balance_server_error"
    ParamType{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_SERVICE_UNAVAILABLE, {}, {} },
      { type::Result::LEDGER_ERROR, {} }
    },
    // "invalid_body_in_balance_server_response"
    ParamType{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_OK, {}, {} },
      { type::Result::LEDGER_ERROR, 0.0 }
    },
    // "happy_path"
    ParamType{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_OK, R"({ "total": 5.0, "spendable": 0.0, "confirmed": 5.0, "unconfirmed": 0.0 })", {} },
      { type::Result::LEDGER_OK, 5.0 }
    }
  ),
  NameSuffixGenerator
);

TEST_P(Get, AnonFunds) {
  const auto& params = GetParam();
  const bool contributions_disabled_for_BAP_migration = std::get<0>(params);
  const bool fetch_old_balance_enabled = std::get<1>(params);
  const std::string& brave_wallet = std::get<2>(params);
  const type::UrlResponse& balance_server_response = std::get<3>(params);
  const auto& expected = std::get<4>(params);

  ON_CALL(
    *mock_ledger_client_,
    GetBooleanOption(option::kContributionsDisabledForBAPMigration)
  ).WillByDefault(
    testing::Return(contributions_disabled_for_BAP_migration)
  );

  ON_CALL(
    *mock_ledger_client_,
    GetBooleanState(state::kFetchOldBalance)
  ).WillByDefault(
    testing::Return(fetch_old_balance_enabled)
  );

  ON_CALL(
    *mock_ledger_client_,
    GetEncryptedStringState(state::kWalletBrave)
  ).WillByDefault(
    testing::Return(brave_wallet)
  );

  ON_CALL(
    *mock_ledger_impl_, database()
  ).WillByDefault(
    testing::Return(mock_database_.get())
  );
  
  ON_CALL(
    *mock_ledger_client_,
    LoadURL(_, _)
  ).WillByDefault(
    Invoke(
      [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        callback(balance_server_response);
      }
    )
  );

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
    }
  );
}

}  // namespace get

namespace transfer {

using Result = type::Result;
using ParamType =
  std::tuple<
    std::string,       // Uphold wallet
    type::UrlResponse, // rewards web services response
    Result             // expected result
  >
;

class Transfer : public TestWithParam<ParamType> {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<UpholdAuthorization> authorization_;

 public:
  Transfer()
    : mock_ledger_client_{ std::make_unique<MockLedgerClient>() }
    , mock_ledger_impl_{ std::make_unique<MockLedgerImpl>(mock_ledger_client_.get()) }
    , authorization_{ std::make_unique<UpholdAuthorization>(mock_ledger_impl_.get()) }
  {}
};

std::string NameSuffixGenerator(const TestParamInfo<Transfer::ParamType>& info) {
  const std::string& uphold_wallet = std::get<0>(info.param);
  const type::UrlResponse& rewards_web_services_response = std::get<1>(info.param);

  if (uphold_wallet.empty()) {
    return "uphold_wallet_is_null";
  }

  if (rewards_web_services_response.status_code == net::HttpStatusCode::HTTP_NOT_FOUND) {
    return "404";
  }

  if (rewards_web_services_response.status_code == net::HttpStatusCode::HTTP_CONFLICT) {
    return "wallet_device_limit_reached";
  }

  if (rewards_web_services_response.status_code != net::HttpStatusCode::HTTP_OK) {
    return "rewards_web_services_error";
  }

  return "happy_path";
}

const std::string uphold_wallet{ R"(
{
    "account_url": "https://wallet-sandbox.uphold.com/dashboard",
    "add_url": "",
    "address": "962df5b1-bb72-4619-a349-c8087941b795",
    "fees": {},
    "login_url": "https://wallet-sandbox.uphold.com/authorize/9a4b665ca983d912fec5c7395b34859a94418cc8?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=025C120562C517028DECD63780FD4B8D48791C4ED8E3747FB2F27445EBFFCFCA",
    "one_time_string": "004603F48D8636C3E96EE049C326976EE432809841EA1E11E305EC5F7C96A1B7",
    "status": 2,
    "token": "0047c2fd8f023e067354dbdb5639ee67acf77150",
    "user_name": "",
    "verify_url": "https://wallet-sandbox.uphold.com/authorize/9a4b665ca983d912fec5c7395b34859a94418cc8?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=025C120562C517028DECD63780FD4B8D48791C4ED8E3747FB2F27445EBFFCFCA",
    "withdraw_url": ""
}
)" };

INSTANTIATE_TEST_SUITE_P(
  UpholdAuthorizationTest,
  Transfer,
  Values(
    // "uphold_wallet_is_null"
    ParamType{
      "",
      type::UrlResponse{},
      type::Result::LEDGER_ERROR
    },
    // "404"
    ParamType{
      uphold_wallet,
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_NOT_FOUND, {}, {} },
      type::Result::NOT_FOUND
    },
    // "wallet_device_limit_reached"
    ParamType{
      uphold_wallet,
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_CONFLICT, {}, {} },
      type::Result::ALREADY_EXISTS
    },
    // "rewards_web_services_error"
    ParamType{
      uphold_wallet,
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR, {}, {} },
      type::Result::LEDGER_ERROR
    },
    // "happy_path"
    ParamType{
      uphold_wallet,
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_OK, {}, {} },
      type::Result::LEDGER_OK
    }
  ),
  NameSuffixGenerator
);

TEST_P(Transfer, AnonFunds) {
  const auto& params = GetParam();
  const std::string& uphold_wallet = std::get<0>(params);
  const type::UrlResponse& rewards_web_services_response = std::get<1>(params);
  const auto& expected_result = std::get<2>(params);

  ON_CALL(
    *mock_ledger_client_,
    GetEncryptedStringState(state::kWalletUphold)
  ).WillByDefault(
    testing::Return(uphold_wallet)
  );
  
  ON_CALL(
    *mock_ledger_client_,
    LoadURL(_, _)
  ).WillByDefault(
    Invoke(
      [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        callback(rewards_web_services_response);
      }
    )
  );

  authorization_->TransferAnonFunds(
    0.0,
    [&](const type::Result result) {
      EXPECT_TRUE(result == expected_result);
    }
  );
}

}  // namespace transfer
}  // namespace anon_funds
}  // namespace tests
}  // namespace uphold
}  // namespace ledger
