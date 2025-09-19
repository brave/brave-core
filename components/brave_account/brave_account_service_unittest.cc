/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

struct RegisterInitializeTestCase {
  using EndpointExpected =
      base::expected<std::optional<endpoints::PasswordInit::Response>,
                     std::optional<endpoints::PasswordInit::Error>>;
  using MojoExpected = base::expected<mojom::RegisterInitializeResultPtr,
                                      mojom::RegisterFailureReason>;

  static void Run(mojom::Authentication& authentication,
                  const RegisterInitializeTestCase& test_case,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication.RegisterInitialize(
        test_case.email, test_case.blinded_message, std::move(callback));
  }

  std::string test_name;
  std::string email;
  std::string blinded_message;
  net::HttpStatusCode http_status_code;
  bool fail_cryptography;
  EndpointExpected endpoint_expected;
  MojoExpected mojo_expected;
};

const RegisterInitializeTestCase kInitializeErrorFailedToParse{
    .test_name = "initialize_error_failed_to_parse",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
    .fail_cryptography = {},  // not used
    .endpoint_expected = base::unexpected(std::nullopt),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected),
};

const RegisterInitializeTestCase kInitializeBadRequest{
    .test_name = "initialize_bad_request",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_BAD_REQUEST,
    .fail_cryptography = {},  // not used
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordInit::Error error;
      error.code = 13004;
      error.error = "account already exists";
      error.status = net::HTTP_BAD_REQUEST;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeBadRequest),
};

const RegisterInitializeTestCase kInitializeUnauthorized{
    .test_name = "initialize_unauthorized",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_UNAUTHORIZED,
    .fail_cryptography = {},  // not used
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordInit::Error error;
      error.code = 0;
      error.error = "Unauthorized";
      error.status = net::HTTP_UNAUTHORIZED;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnauthorized),
};

const RegisterInitializeTestCase kInitializeServerError{
    .test_name = "initialize_server_error",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
    .fail_cryptography = {},  // not used
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordInit::Error error;
      error.code = 0;
      error.error = "Internal Server Error";
      error.status = net::HTTP_INTERNAL_SERVER_ERROR;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeServerError),
};

const RegisterInitializeTestCase kInitializeUnknown{
    .test_name = "initialize_unknown",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_TOO_EARLY,
    .fail_cryptography = {},  // not used
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordInit::Error error;
      error.code = 0;
      error.error = "Too Early";
      error.status = net::HTTP_TOO_EARLY;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnknown),
};

const RegisterInitializeTestCase kInitializeResponseFailedToParse{
    .test_name = "initialize_response_failed_to_parse",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_OK,
    .fail_cryptography = {},  // not used
    .endpoint_expected = base::ok(std::nullopt),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected),
};

const RegisterInitializeTestCase kInitializeVerificationTokenEmpty{
    .test_name = "initialize_verification_token_empty",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_OK,
    .fail_cryptography = {},  // not used
    .endpoint_expected =
        [] {
          endpoints::PasswordInit::Response response;
          response.verification_token = "";
          response.serialized_response = "serialized_response";
          return response;
        }(),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected),
};

const RegisterInitializeTestCase kInitializeSerializedResponseEmpty{
    .test_name = "initialize_serialized_response_empty",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_OK,
    .fail_cryptography = {},  // not used
    .endpoint_expected =
        [] {
          endpoints::PasswordInit::Response response;
          response.verification_token = "verification_token";
          response.serialized_response = "";
          return response;
        }(),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected),
};

const RegisterInitializeTestCase kInitializeVerificationTokenFailedToEncrypt{
    .test_name = "initialize_verification_token_failed_to_encrypt",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_OK,
    .fail_cryptography = true,
    .endpoint_expected =
        [] {
          endpoints::PasswordInit::Response response;
          response.verification_token = "verification_token";
          response.serialized_response = "serialized_response";
          return response;
        }(),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected),
};

const RegisterInitializeTestCase kInitializeSuccess{
    .test_name = "initialize_success",
    .email = "email",
    .blinded_message = "blinded_message",
    .http_status_code = net::HTTP_OK,
    .fail_cryptography = false,
    .endpoint_expected =
        [] {
          endpoints::PasswordInit::Response response;
          response.verification_token = "verification_token";
          response.serialized_response = "serialized_response";
          return response;
        }(),
    .mojo_expected =
        mojom::RegisterInitializeResult::New("verification_token",
                                             "serialized_response"),
};

using RegisterInitializeTest =
    BraveAccountServiceTest<RegisterInitializeTestCase>;

TEST_P(RegisterInitializeTest, MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    RegisterInitializeTest,
    testing::Values(&kInitializeErrorFailedToParse,
                    &kInitializeBadRequest,
                    &kInitializeUnauthorized,
                    &kInitializeServerError,
                    &kInitializeUnknown,
                    &kInitializeResponseFailedToParse,
                    &kInitializeVerificationTokenEmpty,
                    &kInitializeSerializedResponseEmpty,
                    &kInitializeVerificationTokenFailedToEncrypt,
                    &kInitializeSuccess),
    RegisterInitializeTest::kNameGenerator);

struct RegisterFinalizeTestCase {
  using EndpointExpected =
      base::expected<std::optional<endpoints::PasswordFinalize::Response>,
                     std::optional<endpoints::PasswordFinalize::Error>>;
  using MojoExpected = base::expected<mojom::RegisterFinalizeResultPtr,
                                      mojom::RegisterFailureReason>;

  static void Run(mojom::Authentication& authentication,
                  const RegisterFinalizeTestCase& test_case,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication.RegisterFinalize(test_case.encrypted_verification_token,
                                    test_case.serialized_record,
                                    std::move(callback));
  }

  std::string test_name;
  std::string encrypted_verification_token;
  std::string serialized_record;
  bool fail_cryptography;
  net::HttpStatusCode http_status_code;
  EndpointExpected endpoint_expected;
  MojoExpected mojo_expected;
};

const RegisterFinalizeTestCase kFinalizeVerificationTokenFailedToDecrypt{
    .test_name = "finalize_verification_token_failed_to_decrypt",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = true,
    .http_status_code = {},   // not used
    .endpoint_expected = {},  // not used
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnexpected),
};

const RegisterFinalizeTestCase kFinalizeErrorFailedToParse{
    .test_name = "finalize_error_failed_to_parse",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
    .endpoint_expected = base::unexpected(std::nullopt),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnexpected),
};

const RegisterFinalizeTestCase kFinalizeBadRequest{
    .test_name = "finalize_bad_request",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_BAD_REQUEST,
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordFinalize::Error error;
      error.code = 14002;
      error.error = "interim password state has expired";
      error.status = net::HTTP_BAD_REQUEST;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeBadRequest),
};

const RegisterFinalizeTestCase kFinalizeUnauthorized{
    .test_name = "finalize_unauthorized",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_UNAUTHORIZED,
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordFinalize::Error error;
      error.code = 0;
      error.error = "Unauthorized";
      error.status = net::HTTP_UNAUTHORIZED;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnauthorized),
};

const RegisterFinalizeTestCase kFinalizeForbidden{
    .test_name = "finalize_forbidden",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_FORBIDDEN,
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordFinalize::Error error;
      error.code = 0;
      error.error = "Forbidden";
      error.status = net::HTTP_FORBIDDEN;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeForbidden),
};

const RegisterFinalizeTestCase kFinalizeNotFound{
    .test_name = "finalize_not_found",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_NOT_FOUND,
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordFinalize::Error error;
      error.code = 14001;
      error.error = "interim password state not found";
      error.status = net::HTTP_NOT_FOUND;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeNotFound),
};

const RegisterFinalizeTestCase kFinalizeServerError{
    .test_name = "finalize_server_error",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordFinalize::Error error;
      error.code = 0;
      error.error = "Internal Server Error";
      error.status = net::HTTP_INTERNAL_SERVER_ERROR;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeServerError),
};

const RegisterFinalizeTestCase kFinalizeUnknown{
    .test_name = "finalize_unknown",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_TOO_EARLY,
    .endpoint_expected = base::unexpected([] {
      endpoints::PasswordFinalize::Error error;
      error.code = 0;
      error.error = "Too Early";
      error.status = net::HTTP_TOO_EARLY;
      return error;
    }()),
    .mojo_expected =
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnknown),
};

const RegisterFinalizeTestCase kFinalizeSuccess{
    .test_name = "finalize_success",
    .encrypted_verification_token = "encrypted_verification_token",
    .serialized_record = "serialized_record",
    .fail_cryptography = false,
    .http_status_code = net::HTTP_OK,
    .endpoint_expected = endpoints::PasswordFinalize::Response(),
    .mojo_expected = mojom::RegisterFinalizeResult::New(),
};

using RegisterFinalizeTest = BraveAccountServiceTest<RegisterFinalizeTestCase>;

TEST_P(RegisterFinalizeTest, MapsEndpointExpectedToMojoExpected) {
  RunTestCase();

  const auto& test_case = CHECK_DEREF(this->GetParam());
  EXPECT_EQ(!pref_service_.GetString(prefs::kVerificationToken).empty(),
            test_case.mojo_expected.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    RegisterFinalizeTest,
    testing::Values(&kFinalizeVerificationTokenFailedToDecrypt,
                    &kFinalizeErrorFailedToParse,
                    &kFinalizeBadRequest,
                    &kFinalizeUnauthorized,
                    &kFinalizeForbidden,
                    &kFinalizeNotFound,
                    &kFinalizeServerError,
                    &kFinalizeUnknown,
                    &kFinalizeSuccess),
    RegisterFinalizeTest::kNameGenerator);

}  // namespace brave_account
