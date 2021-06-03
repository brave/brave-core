/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_authorization.h"
#include "bat/ledger/option_keys.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=UpholdAuthorizationTest.*
// npm run test -- brave_unit_tests Debug --filter=UpholdAuthorizationTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace uphold {

using param_type =
  std::tuple<
    bool,              // GetBooleanOption(option::kContributionsDisabledForBAPMigration)
    bool,              // GetBooleanState(ledger::state::kFetchOldBalance)
    std::string,       // GetEncryptedStringState(state::kWalletBrave)
    type::UrlResponse, // LoadURL(_, _) - response from the balance server
    type::Result       // expected
  >
;

class Get : public testing::TestWithParam<param_type> {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<UpholdAuthorization> authorization_;

 public:
  Get()
    : mock_ledger_client_{ std::make_unique<ledger::MockLedgerClient>() }
    , mock_ledger_impl_{ std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get()) }
    , authorization_{ std::make_unique<UpholdAuthorization>(mock_ledger_impl_.get()) }
  {}
};

INSTANTIATE_TEST_SUITE_P(
  UpholdAuthorizationTest,
  Get,
  ::testing::Values(
    param_type{
      true,
      {},
      {},
      {},
      type::Result::LEDGER_OK
    },
    param_type{
      false,
      false,
      {},
      {},
      type::Result::LEDGER_OK
    },
    //param_type{
    //  false,
    //  true,
    //  {},
    //  {},
    //  {}
    //},
    param_type{
      false,
      true,
      R"({ "payment_id": "", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      {},
      type::Result::LEDGER_ERROR
    },
    param_type{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_SERVICE_UNAVAILABLE, {}, {} },
      type::Result::LEDGER_ERROR
    },
    param_type{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_OK, {}, {} },
      type::Result::LEDGER_ERROR
    },
    param_type{
      false,
      true,
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{ {}, {}, net::HttpStatusCode::HTTP_OK, R"({ "total": 5.0, "spendable": 0.0, "confirmed": 5.0, "unconfirmed": 0.0 })", {} },
      type::Result::LEDGER_OK
    }
  ),
  [](const ::testing::TestParamInfo<Get::ParamType>& info) -> std::string {
    const auto& params = info.param;
    const bool kContributionsDisabledForBAPMigration = std::get<0>(params);
    const bool kFetchOldBalance = std::get<1>(params);
    const std::string& braveWallet = std::get<2>(params);
    const type::UrlResponse& response = std::get<3>(params);

    if (kContributionsDisabledForBAPMigration) {
      return "fetch_balance_disabled_for_BAP_migration";
    } else if (!kFetchOldBalance) {
      return "get_fetch_old_balance_disabled";
    } else if (braveWallet.empty()) {
      return "wallet_is_not_created";
    } else {
      base::Optional<base::Value> value = base::JSONReader::Read(braveWallet);
      EXPECT_TRUE(value && value->is_dict());
      
      base::DictionaryValue* dictionary = nullptr;
      EXPECT_TRUE(value->GetAsDictionary(&dictionary));
      
      auto* payment_id = dictionary->FindStringKey("payment_id");
      EXPECT_TRUE(payment_id);

      if (payment_id->empty()) {
        return "payment_id_is_empty";
      } else if (response.status_code == net::HttpStatusCode::HTTP_SERVICE_UNAVAILABLE) {
        return "balance_server_error";
      } else if (response.body.empty()) {
        return "invalid_body_in_response";
      } else {
        return "happy_path";
      }
    }
  }
);

TEST_P(Get, AnonFunds) {
  const auto& param = GetParam();
  const bool kContributionsDisabledForBAPMigration = std::get<0>(param);
  const bool kFetchOldBalance = std::get<1>(param);
  const std::string& braveWallet = std::get<2>(param);
  const type::UrlResponse& response = std::get<3>(param);
  const type::Result expected = std::get<4>(param);
  
  ON_CALL(
    *mock_ledger_client_,
    GetBooleanOption(option::kContributionsDisabledForBAPMigration)
  ).WillByDefault(
    testing::Return(kContributionsDisabledForBAPMigration)
  );

  ON_CALL(
    *mock_ledger_client_,
    GetBooleanState(state::kFetchOldBalance)
  ).WillByDefault(
    testing::Return(kFetchOldBalance)
  );

  ON_CALL(
    *mock_ledger_client_,
    GetEncryptedStringState(state::kWalletBrave)
  ).WillByDefault(
    testing::Return(braveWallet)
  );
  
  ON_CALL(
    *mock_ledger_client_,
    LoadURL(_, _)
  ).WillByDefault(
    Invoke(
      [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
        callback(response);
      }
    )
  );

  authorization_->GetAnonFunds(
    [&](const type::Result result, type::BalancePtr balance) {
      EXPECT_TRUE(result == expected);
    }
  );
}

}  // namespace uphold
}  // namespace ledger
