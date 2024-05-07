/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

#include <tuple>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal {
using Result = endpoints::PostConnect::Result;

class ConnectTestWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectTestWallet(RewardsEngine& engine, Result post_connect_result)
      : ConnectExternalWallet(engine),
        post_connect_result_(post_connect_result) {}

  ~ConnectTestWallet() override = default;

 private:
  const char* WalletType() const override { return "uphold"; }

  std::string GetOAuthLoginURL() const override {
    return "https://test.com?" + oauth_info_.one_time_string;
  }

  void Authorize(ConnectExternalWalletCallback callback) override {
    OnConnect(std::move(callback), "token", "address",
              Result(post_connect_result_));
  }

  Result post_connect_result_;
};

using ConnectExternalWalletTestParamType = std::tuple<
    std::string,                               // test name suffix
    mojom::WalletStatus,                       // wallet status
    std::string,                               // one time string
    base::flat_map<std::string, std::string>,  // query parameters
    Result,                                    // post connect result
    mojom::ConnectExternalWalletResult         // expected result
>;

class RewardsConnectExternalWalletTest
    : public RewardsEngineTest,
      public WithParamInterface<ConnectExternalWalletTestParamType> {};

TEST_P(RewardsConnectExternalWalletTest, Paths) {
  const auto& [ignore, wallet_status, one_time_string, query_parameters,
               post_connect_result, expected_result] = GetParam();

  std::string test_wallet = FakeEncryption::Base64EncryptString(
      R"({ "status": )" +
      base::NumberToString(static_cast<int>(wallet_status)) + "}");

  engine().Get<Prefs>().SetString(prefs::kWalletUphold, test_wallet);

  ConnectTestWallet connect_wallet(engine(), post_connect_result);

  engine().GetOptionsForTesting().is_testing = true;
  EXPECT_EQ(connect_wallet.GenerateLoginURL(), "https://test.com?123456789");

  connect_wallet.SetOAuthStateForTesting(
      {.one_time_string = one_time_string, .code_verifier = "", .code = ""});

  auto result = WaitFor<mojom::ConnectExternalWalletResult>([&](auto callback) {
    connect_wallet.Run(query_parameters, std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsConnectExternalWalletTest,
    RewardsConnectExternalWalletTest,
    Values(
        ConnectExternalWalletTestParamType{
            "unexpected_wallet_state",
            mojom::WalletStatus::kConnected,
            "one_time_string",
            {},
            {},
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "query_parameters_error_description_user_does_not_meet_minimum_"
            "requirements",
            mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{
                {"error_description",
                 "User does not meet minimum requirements"}},
            {},
            mojom::ConnectExternalWalletResult::kKYCRequired},
        ConnectExternalWalletTestParamType{
            "query_parameters_error_description_not_available_for_user_"
            "geolocation",
            mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{
                {"error_description", "not available for user geolocation"}},
            {},
            mojom::ConnectExternalWalletResult::kRegionNotSupported},
        ConnectExternalWalletTestParamType{
            "query_parameters_error_description_unknown_error_message",
            mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{
                {"error_description", "unknown error message"}},
            {},
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "query_parameters_code_is_missing",
            mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{{"state", ""}},
            {},
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "query_parameters_state_is_missing",
            mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{{"code", ""}},
            {},
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "query_parameters_one_time_string_mismatch",
            mojom::WalletStatus::kNotConnected,
            "one_time_string_1",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string_2"}},
            {},
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "post_connect_failed_to_create_request",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kFailedToCreateRequest),
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "post_connect_flagged_wallet", mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kFlaggedWallet),
            mojom::ConnectExternalWalletResult::kFlaggedWallet},
        ConnectExternalWalletTestParamType{
            "post_connect_mismatched_countries",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kMismatchedCountries),
            mojom::ConnectExternalWalletResult::kMismatchedCountries},
        ConnectExternalWalletTestParamType{
            "post_connect_provider_unavailable",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kProviderUnavailable),
            mojom::ConnectExternalWalletResult::kProviderUnavailable},
        ConnectExternalWalletTestParamType{
            "post_connect_region_not_supported",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kRegionNotSupported),
            mojom::ConnectExternalWalletResult::kRegionNotSupported},
        ConnectExternalWalletTestParamType{
            "post_connect_unknown_message", mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kUnknownMessage),
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "post_connect_kyc_required", mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kKYCRequired),
            mojom::ConnectExternalWalletResult::kKYCRequired},
        ConnectExternalWalletTestParamType{
            "post_connect_mismatched_provider_accounts",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(
                mojom::PostConnectError::kMismatchedProviderAccounts),
            mojom::ConnectExternalWalletResult::kMismatchedProviderAccounts},
        ConnectExternalWalletTestParamType{
            "post_connect_request_signature_verification_failure",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(
                mojom::PostConnectError::kRequestSignatureVerificationFailure),
            mojom::ConnectExternalWalletResult::
                kRequestSignatureVerificationFailure},
        ConnectExternalWalletTestParamType{
            "post_connect_transaction_verification_failure",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(
                mojom::PostConnectError::kTransactionVerificationFailure),
            mojom::ConnectExternalWalletResult::
                kUpholdTransactionVerificationFailure},
        ConnectExternalWalletTestParamType{
            "post_connect_device_limit_reached",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kDeviceLimitReached),
            mojom::ConnectExternalWalletResult::kDeviceLimitReached},
        ConnectExternalWalletTestParamType{
            "post_connect_unexpected_error", mojom::WalletStatus::kNotConnected,
            "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kUnexpectedError),
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "post_connect_unexpected_status_code",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kUnexpectedStatusCode),
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "post_connect_failed_to_parse_body",
            mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            base::unexpected(mojom::PostConnectError::kFailedToParseBody),
            mojom::ConnectExternalWalletResult::kUnexpected},
        ConnectExternalWalletTestParamType{
            "success", mojom::WalletStatus::kNotConnected, "one_time_string",
            base::flat_map<std::string, std::string>{
                {"code", ""},
                {"state", "one_time_string"}},
            "US", mojom::ConnectExternalWalletResult::kSuccess}),
    [](const TestParamInfo<ConnectExternalWalletTestParamType>& info) {
      return std::get<0>(info.param);
    });

}  // namespace brave_rewards::internal
