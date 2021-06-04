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
namespace anon_funds {

using GetResult = std::pair<type::Result, base::Optional<double>>;
using GetParamType =
  std::tuple<
    bool,              // contributions disabled for BAP migration
    bool,              // fetch old balance enabled
    std::string,       // Brave wallet
    type::UrlResponse, // balance server response
    GetResult          // expected result
  >
;

class Get : public TestWithParam<GetParamType> {
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

std::string GetNameSuffixGenerator(const TestParamInfo<Get::ParamType>& info) {
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
  
  if (balance_server_response.status_code == net::HttpStatusCode::HTTP_SERVICE_UNAVAILABLE) {
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
    GetParamType{
      true,
      {},
      {},
      {},
      { type::Result::LEDGER_OK, 0.0 }
    },
    // "fetch_old_balance_disabled"
    GetParamType{
      false,
      false,
      {},
      {},
      { type::Result::LEDGER_OK, 0.0 }
    },
    // "brave_wallet_is_not_created"
    GetParamType{
      false,
      true,
      {},
      {},
      { type::Result::LEDGER_OK, 0.0 }
    },
    // "brave_wallet_payment_id_is_empty"
    GetParamType{
      false,
      true,
      R"({ "payment_id": "", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      {},
      { type::Result::LEDGER_ERROR, 0.0 }
    },
    // "balance_server_error"
    GetParamType{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_SERVICE_UNAVAILABLE, {}, {} },
      { type::Result::LEDGER_ERROR, {} }
    },
    // "invalid_body_in_balance_server_response"
    GetParamType{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_OK, {}, {} },
      { type::Result::LEDGER_ERROR, 0.0 }
    },
    // "happy_path"
    GetParamType{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_OK, R"({ "total": 5.0, "spendable": 0.0, "confirmed": 5.0, "unconfirmed": 0.0 })", {} },
      { type::Result::LEDGER_OK, 5.0 }
    }
  ),
  GetNameSuffixGenerator
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

}  // anon_funds
}  // namespace uphold
}  // namespace ledger
