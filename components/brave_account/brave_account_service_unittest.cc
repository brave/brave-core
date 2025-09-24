/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
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

const RegisterInitializeTestCase* InitializeBadRequest() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeBadRequest({
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeBadRequest),
      });
  return kInitializeBadRequest.get();
}

const RegisterInitializeTestCase* InitializeUnauthorized() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeUnauthorized({
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeUnauthorized),
      });
  return kInitializeUnauthorized.get();
}

const RegisterInitializeTestCase* InitializeServerError() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeServerError({
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeServerError),
      });
  return kInitializeServerError.get();
}

const RegisterInitializeTestCase* InitializeUnknown() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeUnknown({
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeUnknown),
      });
  return kInitializeUnknown.get();
}

const RegisterInitializeTestCase* InitializeResponseFailedToParse() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeResponseFailedToParse({
          .test_name = "initialize_response_failed_to_parse",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_OK,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::ok(std::nullopt),
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeUnexpected),
      });
  return kInitializeResponseFailedToParse.get();
}

const RegisterInitializeTestCase* InitializeVerificationTokenEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeVerificationTokenEmpty({
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeUnexpected),
      });
  return kInitializeVerificationTokenEmpty.get();
}

const RegisterInitializeTestCase* InitializeSerializedResponseEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeSerializedResponseEmpty({
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeUnexpected),
      });
  return kInitializeSerializedResponseEmpty.get();
}

const RegisterInitializeTestCase* InitializeVerificationTokenFailedToEncrypt() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeVerificationTokenFailedToEncrypt({
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kInitializeUnexpected),
      });
  return kInitializeVerificationTokenFailedToEncrypt.get();
}

const RegisterInitializeTestCase* InitializeSuccess() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeSuccess({
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
          .mojo_expected = mojom::RegisterInitializeResult::New(
              base::Base64Encode("verification_token"), "serialized_response"),
      });
  return kInitializeSuccess.get();
}

using RegisterInitializeTest =
    BraveAccountServiceTest<RegisterInitializeTestCase>;

TEST_P(RegisterInitializeTest, MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    RegisterInitializeTest,
    testing::Values(InitializeBadRequest(),
                    InitializeUnauthorized(),
                    InitializeServerError(),
                    InitializeUnknown(),
                    InitializeResponseFailedToParse(),
                    InitializeVerificationTokenEmpty(),
                    InitializeSerializedResponseEmpty(),
                    InitializeVerificationTokenFailedToEncrypt(),
                    InitializeSuccess()),
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

const RegisterFinalizeTestCase* FinalizeVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeVerificationTokenFailedToDecrypt({
          .test_name = "finalize_verification_token_failed_to_decrypt",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "serialized_record",
          .fail_cryptography = true,
          .http_status_code = {},   // not used
          .endpoint_expected = {},  // not used
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kFinalizeUnexpected),
      });
  return kFinalizeVerificationTokenFailedToDecrypt.get();
}

const RegisterFinalizeTestCase* FinalizeBadRequest() {
  static const base::NoDestructor<RegisterFinalizeTestCase> kFinalizeBadRequest(
      {
          .test_name = "finalize_bad_request",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kFinalizeBadRequest),
      });
  return kFinalizeBadRequest.get();
}

const RegisterFinalizeTestCase* FinalizeUnauthorized() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeUnauthorized({
          .test_name = "finalize_unauthorized",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kFinalizeUnauthorized),
      });
  return kFinalizeUnauthorized.get();
}

const RegisterFinalizeTestCase* FinalizeForbidden() {
  static const base::NoDestructor<RegisterFinalizeTestCase> kFinalizeForbidden({
      .test_name = "finalize_forbidden",
      .encrypted_verification_token =
          base::Base64Encode("encrypted_verification_token"),
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
  });
  return kFinalizeForbidden.get();
}

const RegisterFinalizeTestCase* FinalizeNotFound() {
  static const base::NoDestructor<RegisterFinalizeTestCase> kFinalizeNotFound({
      .test_name = "finalize_not_found",
      .encrypted_verification_token =
          base::Base64Encode("encrypted_verification_token"),
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
  });
  return kFinalizeNotFound.get();
}

const RegisterFinalizeTestCase* FinalizeServerError() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeServerError({
          .test_name = "finalize_server_error",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
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
          .mojo_expected = base::unexpected(
              mojom::RegisterFailureReason::kFinalizeServerError),
      });
  return kFinalizeServerError.get();
}

const RegisterFinalizeTestCase* FinalizeUnknown() {
  static const base::NoDestructor<RegisterFinalizeTestCase> kFinalizeUnknown({
      .test_name = "finalize_unknown",
      .encrypted_verification_token =
          base::Base64Encode("encrypted_verification_token"),
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
  });
  return kFinalizeUnknown.get();
}

const RegisterFinalizeTestCase* FinalizeSuccess() {
  static const base::NoDestructor<RegisterFinalizeTestCase> kFinalizeSuccess({
      .test_name = "finalize_success",
      .encrypted_verification_token =
          base::Base64Encode("encrypted_verification_token"),
      .serialized_record = "serialized_record",
      .fail_cryptography = false,
      .http_status_code = net::HTTP_OK,
      .endpoint_expected = endpoints::PasswordFinalize::Response(),
      .mojo_expected = mojom::RegisterFinalizeResult::New(),
  });
  return kFinalizeSuccess.get();
}

using RegisterFinalizeTest = BraveAccountServiceTest<RegisterFinalizeTestCase>;

TEST_P(RegisterFinalizeTest, MapsEndpointExpectedToMojoExpected) {
  RunTestCase();

  if (const auto& test_case = CHECK_DEREF(this->GetParam());
      test_case.mojo_expected.has_value()) {
    EXPECT_EQ(pref_service_.GetString(prefs::kVerificationToken),
              test_case.encrypted_verification_token);
  }
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    RegisterFinalizeTest,
    testing::Values(FinalizeVerificationTokenFailedToDecrypt(),
                    FinalizeBadRequest(),
                    FinalizeUnauthorized(),
                    FinalizeForbidden(),
                    FinalizeNotFound(),
                    FinalizeServerError(),
                    FinalizeUnknown(),
                    FinalizeSuccess()),
    RegisterFinalizeTest::kNameGenerator);

}  // namespace brave_account
