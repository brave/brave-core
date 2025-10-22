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
                                      mojom::RegisterErrorPtr>;

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

const RegisterInitializeTestCase* InitializeEmailEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeEmailEmpty({
          .test_name = "initialize_email_empty",
          .email = "",
          .blinded_message = {},    // not used
          .http_status_code = {},   // not used
          .fail_cryptography = {},  // not used
          .endpoint_expected = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kInitializeEmailEmpty.get();
}

const RegisterInitializeTestCase* InitializeBlindedMessageEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeBlindedMessageEmpty({
          .test_name = "initialize_blinded_message_empty",
          .email = "email",
          .blinded_message = "",
          .http_status_code = {},   // not used
          .fail_cryptography = {},  // not used
          .endpoint_expected = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kInitializeBlindedMessageEmpty.get();
}

const RegisterInitializeTestCase* InitializeErrorMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeErrorMissingOrFailedToParse({
          .test_name = "initialize_error_missing_or_failed_to_parse",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::unexpected(std::nullopt),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_INTERNAL_SERVER_ERROR, std::nullopt)),
      });
  return kInitializeErrorMissingOrFailedToParse.get();
}

const RegisterInitializeTestCase* InitializeErrorCodeIsNull() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeErrorCodeIsNull({
          .test_name = "initialize_error_code_is_null",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_BAD_REQUEST,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordInit::Error error;
            error.code = base::Value();
            return error;
          }()),
          .mojo_expected = base::unexpected(
              mojom::RegisterError::New(net::HTTP_BAD_REQUEST, std::nullopt)),
      });
  return kInitializeErrorCodeIsNull.get();
}

const RegisterInitializeTestCase* InitializeNewAccountEmailRequired() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeNewAccountEmailRequired({
          .test_name = "initialize_new_account_email_required",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_BAD_REQUEST,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordInit::Error error;
            error.code = base::Value(11005);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_BAD_REQUEST,
              mojom::RegisterErrorCode::kNewAccountEmailRequired)),
      });
  return kInitializeNewAccountEmailRequired.get();
}

const RegisterInitializeTestCase* InitializeIntentNotAllowed() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeIntentNotAllowed({
          .test_name = "initialize_intent_not_allowed",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_BAD_REQUEST,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordInit::Error error;
            error.code = base::Value(13003);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_BAD_REQUEST,
              mojom::RegisterErrorCode::kIntentNotAllowed)),
      });
  return kInitializeIntentNotAllowed.get();
}

const RegisterInitializeTestCase* InitializeTooManyVerifications() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeTooManyVerifications({
          .test_name = "initialize_too_many_verifications",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_BAD_REQUEST,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordInit::Error error;
            error.code = base::Value(13001);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_BAD_REQUEST,
              mojom::RegisterErrorCode::kTooManyVerifications)),
      });
  return kInitializeTooManyVerifications.get();
}

const RegisterInitializeTestCase* InitializeAccountExists() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeAccountExists({
          .test_name = "initialize_account_exists",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_BAD_REQUEST,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordInit::Error error;
            error.code = base::Value(13004);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_BAD_REQUEST, mojom::RegisterErrorCode::kAccountExists)),
      });
  return kInitializeAccountExists.get();
}

const RegisterInitializeTestCase* InitializeEmailDomainNotSupported() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeEmailDomainNotSupported({
          .test_name = "initialize_email_domain_not_supported",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_BAD_REQUEST,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordInit::Error error;
            error.code = base::Value(13006);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_BAD_REQUEST,
              mojom::RegisterErrorCode::kEmailDomainNotSupported)),
      });
  return kInitializeEmailDomainNotSupported.get();
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
            error.code = base::Value(0);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::RegisterErrorCode::kMiscServerError)),
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
            error.code = base::Value(0);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_INTERNAL_SERVER_ERROR,
              mojom::RegisterErrorCode::kMiscServerError)),
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
            error.code = base::Value(42);
            return error;
          }()),
          .mojo_expected = base::unexpected(
              mojom::RegisterError::New(net::HTTP_TOO_EARLY, std::nullopt)),
      });
  return kInitializeUnknown.get();
}

const RegisterInitializeTestCase* InitializeResponseMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeResponseMissingOrFailedToParse({
          .test_name = "initialize_response_missing_or_failed_to_parse",
          .email = "email",
          .blinded_message = "blinded_message",
          .http_status_code = net::HTTP_OK,
          .fail_cryptography = {},  // not used
          .endpoint_expected = base::ok(std::nullopt),
          .mojo_expected = base::unexpected(
              mojom::RegisterError::New(net::HTTP_OK, std::nullopt)),
      });
  return kInitializeResponseMissingOrFailedToParse.get();
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
              mojom::RegisterError::New(net::HTTP_OK, std::nullopt)),
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
              mojom::RegisterError::New(net::HTTP_OK, std::nullopt)),
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
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              std::nullopt,
              mojom::RegisterErrorCode::kVerificationTokenEncryptionFailed)),
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
    testing::Values(InitializeEmailEmpty(),
                    InitializeBlindedMessageEmpty(),
                    InitializeErrorMissingOrFailedToParse(),
                    InitializeErrorCodeIsNull(),
                    InitializeNewAccountEmailRequired(),
                    InitializeIntentNotAllowed(),
                    InitializeTooManyVerifications(),
                    InitializeAccountExists(),
                    InitializeEmailDomainNotSupported(),
                    InitializeUnauthorized(),
                    InitializeServerError(),
                    InitializeUnknown(),
                    InitializeResponseMissingOrFailedToParse(),
                    InitializeVerificationTokenEmpty(),
                    InitializeSerializedResponseEmpty(),
                    InitializeVerificationTokenFailedToEncrypt(),
                    InitializeSuccess()),
    RegisterInitializeTest::kNameGenerator);

struct RegisterFinalizeTestCase {
  using EndpointExpected =
      base::expected<std::optional<endpoints::PasswordFinalize::Response>,
                     std::optional<endpoints::PasswordFinalize::Error>>;
  using MojoExpected =
      base::expected<mojom::RegisterFinalizeResultPtr, mojom::RegisterErrorPtr>;

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

const RegisterFinalizeTestCase* FinalizeEncryptedVerificationTokenEmpty() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeEncryptedVerificationTokenEmpty({
          .test_name = "finalize_encrypted_verification_token_empty",
          .encrypted_verification_token = "",
          .serialized_record = {},  // not used
          .fail_cryptography = {},  // not used
          .http_status_code = {},   // not used
          .endpoint_expected = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kFinalizeEncryptedVerificationTokenEmpty.get();
}

const RegisterFinalizeTestCase* FinalizeSerializedRecordEmpty() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeSerializedRecordEmpty({
          .test_name = "finalize_serialized_record_empty",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "",
          .fail_cryptography = {},  // not used
          .http_status_code = {},   // not used
          .endpoint_expected = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kFinalizeSerializedRecordEmpty.get();
}

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
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              std::nullopt,
              mojom::RegisterErrorCode::kVerificationTokenDecryptionFailed)),
      });
  return kFinalizeVerificationTokenFailedToDecrypt.get();
}

const RegisterFinalizeTestCase* FinalizeErrorMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeErrorMissingOrFailedToParse({
          .test_name = "finalize_error_missing_or_failed_to_parse",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "serialized_record",
          .fail_cryptography = false,
          .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
          .endpoint_expected = base::unexpected(std::nullopt),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_INTERNAL_SERVER_ERROR, std::nullopt)),
      });
  return kFinalizeErrorMissingOrFailedToParse.get();
}

const RegisterFinalizeTestCase* FinalizeErrorCodeIsNull() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeErrorCodeIsNull({
          .test_name = "finalize_error_code_is_null",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "serialized_record",
          .fail_cryptography = false,
          .http_status_code = net::HTTP_NOT_FOUND,
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordFinalize::Error error;
            error.code = base::Value();
            return error;
          }()),
          .mojo_expected = base::unexpected(
              mojom::RegisterError::New(net::HTTP_NOT_FOUND, std::nullopt)),
      });
  return kFinalizeErrorCodeIsNull.get();
}

const RegisterFinalizeTestCase* FinalizeInterimPasswordStateNotFound() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeInterimPasswordStateNotFound({
          .test_name = "finalize_interim_password_state_not_found",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "serialized_record",
          .fail_cryptography = false,
          .http_status_code = net::HTTP_NOT_FOUND,
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordFinalize::Error error;
            error.code = base::Value(14001);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_NOT_FOUND,
              mojom::RegisterErrorCode::kInterimPasswordStateNotFound)),
      });
  return kFinalizeInterimPasswordStateNotFound.get();
}

const RegisterFinalizeTestCase* FinalizeInterimPasswordStateExpired() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeInterimPasswordStateExpired({
          .test_name = "finalize_interim_password_state_expired",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "serialized_record",
          .fail_cryptography = false,
          .http_status_code = net::HTTP_BAD_REQUEST,
          .endpoint_expected = base::unexpected([] {
            endpoints::PasswordFinalize::Error error;
            error.code = base::Value(14002);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_BAD_REQUEST,
              mojom::RegisterErrorCode::kInterimPasswordStateExpired)),
      });
  return kFinalizeInterimPasswordStateExpired.get();
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
            error.code = base::Value(0);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::RegisterErrorCode::kMiscServerError)),
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
        error.code = base::Value(0);
        return error;
      }()),
      .mojo_expected = base::unexpected(mojom::RegisterError::New(
          net::HTTP_FORBIDDEN, mojom::RegisterErrorCode::kMiscServerError)),
  });
  return kFinalizeForbidden.get();
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
            error.code = base::Value(0);
            return error;
          }()),
          .mojo_expected = base::unexpected(mojom::RegisterError::New(
              net::HTTP_INTERNAL_SERVER_ERROR,
              mojom::RegisterErrorCode::kMiscServerError)),
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
        error.code = base::Value(42);
        return error;
      }()),
      .mojo_expected = base::unexpected(
          mojom::RegisterError::New(net::HTTP_TOO_EARLY, std::nullopt)),
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
    testing::Values(FinalizeEncryptedVerificationTokenEmpty(),
                    FinalizeSerializedRecordEmpty(),
                    FinalizeVerificationTokenFailedToDecrypt(),
                    FinalizeErrorMissingOrFailedToParse(),
                    FinalizeErrorCodeIsNull(),
                    FinalizeInterimPasswordStateNotFound(),
                    FinalizeInterimPasswordStateExpired(),
                    FinalizeUnauthorized(),
                    FinalizeForbidden(),
                    FinalizeServerError(),
                    FinalizeUnknown(),
                    FinalizeSuccess()),
    RegisterFinalizeTest::kNameGenerator);

}  // namespace brave_account
