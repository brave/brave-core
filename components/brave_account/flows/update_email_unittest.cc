/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/update_email.h"

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/auth_validate.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::AuthValidate;

struct UpdateEmailTestCase {
  using Endpoint = AuthValidate;
  using EndpointResponse = Endpoint::Response;

  static void Run(const UpdateEmailTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  BraveAccountService& brave_account_service) {
    AccountStatePrefs account_state_prefs(pref_service);
    account_state_prefs.SetLoggedIn(kEmailAddress,
                                    EncryptedAuthenticationToken());

    task_environment.FastForwardBy(kAuthValidatePollInterval -
                                   base::Seconds(1));

    const auto state = account_state_prefs.GetAccountState();
    if (test_case.expected_authentication_token.empty()) {
      EXPECT_TRUE(state->is_logged_out());
    } else {
      ASSERT_TRUE(state->is_logged_in());
      EXPECT_EQ(state->get_logged_in()->email, test_case.expected_email);
      EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                test_case.expected_authentication_token);
    }

    base::OneShotTimer* timer =
        brave_account_service.UpdateEmailTimerForTesting();
    if (test_case.expected_timer_delay.is_zero()) {
      if (timer) {
        EXPECT_FALSE(timer->IsRunning());
      }
    } else {
      ASSERT_TRUE(timer);
      EXPECT_TRUE(timer->IsRunning());
      EXPECT_EQ(timer->GetCurrentDelay(), test_case.expected_timer_delay);
    }
  }

  std::string test_name;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  std::string expected_email;
  std::string expected_authentication_token;
  base::TimeDelta expected_timer_delay;
};

namespace {

const UpdateEmailTestCase* UpdateEmailAuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<UpdateEmailTestCase> kTestCase({
      .test_name = "update_email_authentication_token_decryption_failed",
      .fail_decryption = true,
      .endpoint_response = {},  // not used
      .expected_email = kEmailAddress,
      .expected_authentication_token = EncryptedAuthenticationToken(),
      .expected_timer_delay = {},
  });
  return kTestCase.get();
}

const UpdateEmailTestCase* UpdateEmailBodyMissingOrFailedToParse() {
  static const base::NoDestructor<UpdateEmailTestCase> kTestCase({
      .test_name = "update_email_body_missing_or_failed_to_parse",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = {}}},
      .expected_email = kEmailAddress,
      .expected_authentication_token = EncryptedAuthenticationToken(),
      .expected_timer_delay = kAuthValidatePollInterval,
  });
  return kTestCase.get();
}

const UpdateEmailTestCase* UpdateEmailEmailEmpty() {
  static const base::NoDestructor<UpdateEmailTestCase> kTestCase({
      .test_name = "update_email_email_empty",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   AuthValidate::Response::SuccessBody body;
                                   body.email = "";
                                   return body;
                                 }()}},
      .expected_email = kEmailAddress,
      .expected_authentication_token = EncryptedAuthenticationToken(),
      .expected_timer_delay = kAuthValidatePollInterval,
  });
  return kTestCase.get();
}

const UpdateEmailTestCase* UpdateEmailSuccess() {
  static const base::NoDestructor<UpdateEmailTestCase> kTestCase({
      .test_name = "update_email_success",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   AuthValidate::Response::SuccessBody body;
                                   body.email = "email_from_response";
                                   return body;
                                 }()}},
      .expected_email = "email_from_response",
      .expected_authentication_token = EncryptedAuthenticationToken(),
      .expected_timer_delay = kAuthValidatePollInterval,
  });
  return kTestCase.get();
}

const UpdateEmailTestCase* UpdateEmail4xx() {
  static const base::NoDestructor<UpdateEmailTestCase> kTestCase({
      .test_name = "update_email_4xx",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_UNAUTHORIZED,
                             .body = base::unexpected([] {
                               AuthValidate::Response::ErrorBody body;
                               body.code = base::Value();
                               return body;
                             }())}},
      .expected_email = "",
      .expected_authentication_token = "",
      .expected_timer_delay = {},
  });
  return kTestCase.get();
}

const UpdateEmailTestCase* UpdateEmail5xx() {
  static const base::NoDestructor<UpdateEmailTestCase> kTestCase({
      .test_name = "update_email_5xx",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = base::unexpected([] {
                               AuthValidate::Response::ErrorBody body;
                               body.code = base::Value();
                               return body;
                             }())}},
      .expected_email = kEmailAddress,
      .expected_authentication_token = EncryptedAuthenticationToken(),
      .expected_timer_delay = kAuthValidatePollInterval,
  });
  return kTestCase.get();
}

using BraveAccountServiceUpdateEmailTest =
    BraveAccountServiceTest<UpdateEmailTestCase>;

}  // namespace

TEST_P(BraveAccountServiceUpdateEmailTest, HandlesUpdateEmailOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceUpdateEmailTest,
    testing::Values(UpdateEmailAuthenticationTokenDecryptionFailed(),
                    UpdateEmailBodyMissingOrFailedToParse(),
                    UpdateEmailEmailEmpty(),
                    UpdateEmailSuccess(),
                    UpdateEmail4xx(),
                    UpdateEmail5xx()),
    BraveAccountServiceUpdateEmailTest::kNameGenerator);

}  // namespace brave_account
