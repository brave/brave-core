/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_map.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/endpoints/post_connect/post_connect.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*ConnectExternalWalletTest*

using testing::Return;
using testing::TestParamInfo;
using testing::TestWithParam;
using testing::Values;

namespace ledger::flows::test {
using Result = endpoints::PostConnect::Result;

class ConnectTestWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectTestWallet(LedgerImpl* ledger, Result post_connect_result)
      : ConnectExternalWallet(ledger),
        post_connect_result_(post_connect_result) {}

  ~ConnectTestWallet() override = default;

 private:
  const char* WalletType() const override { return "test"; }

  void Authorize(OAuthInfo&&, ledger::ConnectExternalWalletCallback callback)
      const override {
    OnConnect(std::move(callback), "token", "address",
              Result(post_connect_result_));
  }

  Result post_connect_result_;
};

// clang-format off
using ConnectExternalWalletTestParamType = std::tuple<
    std::string,                               // test name suffix
    mojom::WalletStatus,                       // wallet status
    std::string,                               // one time string
    base::flat_map<std::string, std::string>,  // query parameters
    Result,                                    // post connect result
    ConnectExternalWalletResult                // expected result
>;
// clang-format on

class ConnectExternalWalletTest
    : public TestWithParam<ConnectExternalWalletTestParamType> {
 public:
  ConnectExternalWalletTest(const ConnectExternalWalletTest&) = delete;
  ConnectExternalWalletTest& operator=(const ConnectExternalWalletTest&) =
      delete;

  ConnectExternalWalletTest(ConnectExternalWalletTest&&) = delete;
  ConnectExternalWalletTest& operator=(ConnectExternalWalletTest&&) = delete;

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  ConnectExternalWalletTest()
      : mock_ledger_client_(),
        mock_ledger_impl_(&mock_ledger_client_),
        mock_database_(&mock_ledger_impl_) {}

  void SetUp() override {
    ON_CALL(mock_ledger_impl_, database())
        .WillByDefault(Return(&mock_database_));
  }

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
  database::MockDatabase mock_database_;
};

TEST_P(ConnectExternalWalletTest, Paths) {
  const auto& [ignore, wallet_status, one_time_string, query_parameters,
               post_connect_result, expected_result] = GetParam();

  std::string test_wallet = FakeEncryption::Base64EncryptString(
      R"({ "one_time_string": ")" + one_time_string + R"(",
          "status": )" +
      std::to_string(static_cast<int>(wallet_status)) + "}");

  ON_CALL(mock_ledger_client_, GetStringState("wallets.test"))
      .WillByDefault(Return(std::move(test_wallet)));

  ConnectTestWallet(&mock_ledger_impl_, post_connect_result)
      .Run(query_parameters,
           base::BindLambdaForTesting([expected_result = expected_result](
                                          ConnectExternalWalletResult result) {
             EXPECT_EQ(result, expected_result);
           }));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Flows,
  ConnectExternalWalletTest,
  Values(
    ConnectExternalWalletTestParamType{
      "unexpected_wallet_state",
      mojom::WalletStatus::kConnected,
      "",
      {},
      {},
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_error_description_user_does_not_meet_minimum_requirements",
      mojom::WalletStatus::kNotConnected,
      "",
      base::flat_map<std::string, std::string>{
        {"error_description", "User does not meet minimum requirements"}
      },
      {},
      base::unexpected(mojom::ConnectExternalWalletError::kKYCRequired)
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_error_description_not_available_for_user_geolocation",
      mojom::WalletStatus::kNotConnected,
      "",
      base::flat_map<std::string, std::string>{
        {"error_description", "not available for user geolocation"}
      },
      {},
      base::unexpected(mojom::ConnectExternalWalletError::kRegionNotSupported)
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_error_description_unknown_error_message",
      mojom::WalletStatus::kNotConnected,
      "",
      base::flat_map<std::string, std::string>{
        {"error_description", "unknown error message"}
      },
      {},
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_code_is_missing",
      mojom::WalletStatus::kNotConnected,
      "",
      base::flat_map<std::string, std::string>{
        {"state", ""}
      },
      {},
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_state_is_missing",
      mojom::WalletStatus::kNotConnected,
      "",
      base::flat_map<std::string, std::string>{
        {"code", ""}
      },
      {},
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_one_time_string_mismatch",
      mojom::WalletStatus::kNotConnected,
      "one_time_string_1",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string_2"}
      },
      {},
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_failed_to_create_request",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kFailedToCreateRequest),
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_flagged_wallet",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kFlaggedWallet),
      base::unexpected(mojom::ConnectExternalWalletError::kFlaggedWallet)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_mismatched_countries",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kMismatchedCountries),
      base::unexpected(mojom::ConnectExternalWalletError::kMismatchedCountries)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_provider_unavailable",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kProviderUnavailable),
      base::unexpected(mojom::ConnectExternalWalletError::kProviderUnavailable)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_region_not_supported",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kRegionNotSupported),
      base::unexpected(mojom::ConnectExternalWalletError::kRegionNotSupported)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_unknown_message",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kUnknownMessage),
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_kyc_required",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kKYCRequired),
      base::unexpected(mojom::ConnectExternalWalletError::kKYCRequired)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_mismatched_provider_accounts",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kMismatchedProviderAccounts),
      base::unexpected(mojom::ConnectExternalWalletError::kMismatchedProviderAccounts)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_request_signature_verification_failure",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kRequestSignatureVerificationFailure),
      base::unexpected(mojom::ConnectExternalWalletError::kRequestSignatureVerificationFailure)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_transaction_verification_failure",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kTransactionVerificationFailure),
      base::unexpected(mojom::ConnectExternalWalletError::kUpholdTransactionVerificationFailure)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_device_limit_reached",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kDeviceLimitReached),
      base::unexpected(mojom::ConnectExternalWalletError::kDeviceLimitReached)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_unexpected_error",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kUnexpectedError),
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_unexpected_status_code",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kUnexpectedStatusCode),
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "post_connect_failed_to_parse_body",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      base::unexpected(mojom::PostConnectError::kFailedToParseBody),
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected)
    },
    ConnectExternalWalletTestParamType{
      "success",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      {},
      {}
    }
  ),
  [](const TestParamInfo<ConnectExternalWalletTestParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace ledger::flows::test
