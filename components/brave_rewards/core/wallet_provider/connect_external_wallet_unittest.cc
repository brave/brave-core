/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <tuple>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/database/database_mock.h"
#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*ConnectExternalWalletTest*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::flows::test {
using Result = endpoints::PostConnect::Result;

class ConnectTestWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectTestWallet(RewardsEngineImpl& engine,
                             Result post_connect_result)
      : ConnectExternalWallet(engine),
        post_connect_result_(post_connect_result) {}

  ~ConnectTestWallet() override = default;

 private:
  const char* WalletType() const override { return "test"; }

  std::string GetOAuthLoginURL() const override {
    return "https://test.com?" + oauth_info_.one_time_string;
  }

  void Authorize(ConnectExternalWalletCallback callback) override {
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
    mojom::ConnectExternalWalletResult         // expected result
>;
// clang-format on

class ConnectExternalWalletTest
    : public TestWithParam<ConnectExternalWalletTestParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(ConnectExternalWalletTest, Paths) {
  const auto& [ignore, wallet_status, one_time_string, query_parameters,
               post_connect_result, expected_result] = GetParam();

  std::string test_wallet = FakeEncryption::Base64EncryptString(
      R"({ "status": )" +
      base::NumberToString(static_cast<int>(wallet_status)) + "}");

  ON_CALL(*mock_engine_impl_.mock_client(), GetStringState("wallets.test", _))
      .WillByDefault([&](const std::string&, auto callback) {
        std::move(callback).Run(test_wallet);
      });

  ON_CALL(*mock_engine_impl_.mock_client(), RunDBTransaction(_, _))
      .WillByDefault([](mojom::DBTransactionPtr transaction, auto callback) {
        std::move(callback).Run(db_error_response->Clone());
      });

  base::MockCallback<ConnectExternalWalletCallback> callback;
  EXPECT_CALL(callback, Run(expected_result)).Times(1);

  ConnectTestWallet connect_wallet(mock_engine_impl_, post_connect_result);

  mock_engine_impl_.GetOptionsForTesting().is_testing = true;
  EXPECT_EQ(connect_wallet.GenerateLoginURL(), "https://test.com?123456789");

  connect_wallet.SetOAuthStateForTesting(
      {.one_time_string = one_time_string, .code_verifier = "", .code = ""});

  connect_wallet.Run(query_parameters, callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Flows,
  ConnectExternalWalletTest,
  Values(
    ConnectExternalWalletTestParamType{
      "unexpected_wallet_state",
      mojom::WalletStatus::kConnected,
      "one_time_string",
      {},
      {},
      mojom::ConnectExternalWalletResult::kUnexpected
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_error_description_user_does_not_meet_minimum_requirements",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"error_description", "User does not meet minimum requirements"}
      },
      {},
      mojom::ConnectExternalWalletResult::kKYCRequired
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_error_description_not_available_for_user_geolocation",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"error_description", "not available for user geolocation"}
      },
      {},
      mojom::ConnectExternalWalletResult::kRegionNotSupported
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_error_description_unknown_error_message",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"error_description", "unknown error message"}
      },
      {},
      mojom::ConnectExternalWalletResult::kUnexpected
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_code_is_missing",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"state", ""}
      },
      {},
      mojom::ConnectExternalWalletResult::kUnexpected
    },
    ConnectExternalWalletTestParamType{
      "query_parameters_state_is_missing",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""}
      },
      {},
      mojom::ConnectExternalWalletResult::kUnexpected
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
      mojom::ConnectExternalWalletResult::kUnexpected
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
      mojom::ConnectExternalWalletResult::kUnexpected
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
      mojom::ConnectExternalWalletResult::kFlaggedWallet
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
      mojom::ConnectExternalWalletResult::kMismatchedCountries
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
      mojom::ConnectExternalWalletResult::kProviderUnavailable
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
      mojom::ConnectExternalWalletResult::kRegionNotSupported
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
      mojom::ConnectExternalWalletResult::kUnexpected
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
      mojom::ConnectExternalWalletResult::kKYCRequired
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
      mojom::ConnectExternalWalletResult::kMismatchedProviderAccounts
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
      mojom::ConnectExternalWalletResult::kRequestSignatureVerificationFailure
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
      mojom::ConnectExternalWalletResult::kUpholdTransactionVerificationFailure
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
      mojom::ConnectExternalWalletResult::kDeviceLimitReached
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
      mojom::ConnectExternalWalletResult::kUnexpected
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
      mojom::ConnectExternalWalletResult::kUnexpected
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
      mojom::ConnectExternalWalletResult::kUnexpected
    },
    ConnectExternalWalletTestParamType{
      "success",
      mojom::WalletStatus::kNotConnected,
      "one_time_string",
      base::flat_map<std::string, std::string>{
        {"code", ""},
        {"state", "one_time_string"}
      },
      "US",
      mojom::ConnectExternalWalletResult::kSuccess
    }
  ),
  [](const TestParamInfo<ConnectExternalWalletTestParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::flows::test
