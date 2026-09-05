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
                  base::test::TaskEnvironment&,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    AccountStatePrefs account_state_prefs(pref_service);
    if (test_case.is_logged_in) {
      account_state_prefs.SetLoggedIn(kEmailAddress,
                                      EncryptedAuthenticationToken());
    }

    authentication->DeleteAccount(std::move(callback).Then(base::BindOnce(
        [](PrefService* pref_service, bool is_logged_in, bool is_success) {
          const auto state = AccountStatePrefs(*pref_service).GetAccountState();
          if (is_success) {
            ASSERT_TRUE(state->is_logged_out());
            EXPECT_FALSE(state->get_logged_out()->verification);
          } else if (is_logged_in) {
            ASSERT_TRUE(state->is_logged_in());
          } else {
            ASSERT_TRUE(state->is_logged_out());
          }
        },
        base::Unretained(&pref_service), test_case.is_logged_in,
        test_case.mojo_expected.has_value())));
  }

  std::string test_name;
  bool fail_decryption = false;
  bool is_logged_in = true;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const DeleteAccountTestCase*
DeleteAccountAuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_authentication_token_decryption_failed",
      .fail_decryption = true,
      .is_logged_in = true,
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
      .is_logged_in = true,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_NO_CONTENT,
                             .body = std::nullopt}},
      .mojo_expected = mojom::DeleteAccountResult::New(),
  });
  return kTestCase.get();
}

const DeleteAccountTestCase* DeleteAccountError() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_error",
      .fail_decryption = false,
      .is_logged_in = true,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_UNAUTHORIZED,
                             .body = base::unexpected(
                                 AccountsDelete::Response::ErrorBody())}},
      .mojo_expected =
          base::unexpected(mojom::DeleteAccountError::NewServerError(
              mojom::DeleteAccountServerError::New(
                  net::HTTP_UNAUTHORIZED,
                  mojom::DeleteAccountServerErrorCode::kNull))),
  });
  return kTestCase.get();
}

const DeleteAccountTestCase* DeleteAccountCalledInWrongState() {
  static const base::NoDestructor<DeleteAccountTestCase> kTestCase({
      .test_name = "delete_account_called_in_wrong_state",
      .fail_decryption = {},  // not used
      .is_logged_in = false,
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::DeleteAccountError::NewClientError(
              mojom::DeleteAccountClientError::New(
                  mojom::DeleteAccountClientErrorCode::kCalledInWrongState))),
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
                    DeleteAccountError(),
                    DeleteAccountCalledInWrongState()),
    BraveAccountServiceDeleteAccountTest::kNameGenerator);

}  // namespace brave_account
