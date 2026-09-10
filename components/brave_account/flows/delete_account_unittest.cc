/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/delete_account.h"

#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/accounts_delete.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/delete_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::AccountsDelete;

struct DeleteAccountTestCase {
  using Endpoint = AccountsDelete;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::DeleteAccountResultPtr,
                                      mojom::DeleteAccountErrorPtr>;

  static void Run(const DeleteAccountTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    AccountStatePrefs(pref_service)
        .SetLoggedIn(kEmailAddress, EncryptedAuthenticationToken());

    authentication->DeleteAccount(std::move(callback).Then(base::BindOnce(
        [](PrefService* pref_service, bool success) {
          AccountStatePrefs account_state_prefs(*pref_service);
          const auto state = account_state_prefs.GetAccountState();
          if (success) {
            // LoggedIn ==> LoggedOut (state swap).
            ASSERT_TRUE(state->is_logged_out());
            EXPECT_FALSE(state->get_logged_out()->verification);
          } else {
            // The logged-in email and authentication token are left intact.
            ASSERT_TRUE(state->is_logged_in());
            EXPECT_FALSE(state->get_logged_in()->verification);
            EXPECT_EQ(state->get_logged_in()->email, kEmailAddress);
            EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                      EncryptedAuthenticationToken());
          }
        },
        base::Unretained(&pref_service), test_case.mojo_expected.has_value())));
  }

  std::string test_name;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const DeleteAccountTestCase*
DeleteAccountAuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_authentication_token_decryption_failed",
      .fail_decryption = true,
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::DeleteAccountError::NewClientError(
              mojom::DeleteAccountClientError::New(
                  mojom::DeleteAccountClientErrorCode::
                      kAuthenticationTokenDecryptionFailed))),
  });
  return kTestCase.get();
}

const DeleteAccountTestCase* DeleteAccountSuccess() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_success",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_NO_CONTENT,
                             .body = std::nullopt}},
      .mojo_expected = mojom::DeleteAccountResult::New(),
  });
  return kTestCase.get();
}

const DeleteAccountTestCase* DeleteAccountBodyMissingOrFailedToParse() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_body_missing_or_failed_to_parse",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}},
      .mojo_expected =
          base::unexpected(mojom::DeleteAccountError::NewServerError(
              mojom::DeleteAccountServerError::New(
                  net::HTTP_INTERNAL_SERVER_ERROR,
                  mojom::DeleteAccountServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const DeleteAccountTestCase* DeleteAccountUnexpectedSuccessBody() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_unexpected_success_body",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body = AccountsDelete::Response::SuccessBody()}},
      .mojo_expected =
          base::unexpected(mojom::DeleteAccountError::NewServerError(
              mojom::DeleteAccountServerError::New(
                  net::HTTP_OK,
                  mojom::DeleteAccountServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const DeleteAccountTestCase* DeleteAccountErrorCodeIsNull() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_error_code_is_null",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_UNAUTHORIZED,
                             .body = base::unexpected([] {
                               AccountsDelete::Response::ErrorBody body;
                               body.code = base::Value();
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::DeleteAccountError::NewServerError(
              mojom::DeleteAccountServerError::New(
                  net::HTTP_UNAUTHORIZED,
                  mojom::DeleteAccountServerErrorCode::kNull))),
  });
  return kTestCase.get();
}

const DeleteAccountTestCase* DeleteAccountUnknownErrorCode() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_unknown_error_code",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_TOO_EARLY,
                             .body = base::unexpected([] {
                               AccountsDelete::Response::ErrorBody body;
                               body.code = base::Value(42);
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::DeleteAccountError::NewServerError(
              mojom::DeleteAccountServerError::New(
                  net::HTTP_TOO_EARLY,
                  mojom::DeleteAccountServerErrorCode::kUnknown))),
  });
  return kTestCase.get();
}

using BraveAccountServiceDeleteAccountTest =
    BraveAccountServiceTest<DeleteAccountTestCase>;

}  // namespace

TEST_P(BraveAccountServiceDeleteAccountTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceDeleteAccountTest,
    testing::Values(DeleteAccountAuthenticationTokenDecryptionFailed(),
                    DeleteAccountSuccess(),
                    DeleteAccountBodyMissingOrFailedToParse(),
                    DeleteAccountUnexpectedSuccessBody(),
                    DeleteAccountErrorCodeIsNull(),
                    DeleteAccountUnknownErrorCode()),
    BraveAccountServiceDeleteAccountTest::kNameGenerator);

}  // namespace brave_account
