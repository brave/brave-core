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
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/verify_result.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyResult;

struct RegisterInitializeTestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::RegisterInitializeResultPtr,
                                      mojom::RegisterErrorPtr>;

  static void Run(const RegisterInitializeTestCase& test_case,
                  mojom::Authentication& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication.RegisterInitialize(
        test_case.email, test_case.blinded_message, std::move(callback));
  }

  std::string test_name;
  std::string email;
  std::string blinded_message;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const RegisterInitializeTestCase* InitializeEmailEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kInitializeEmailEmpty({
          .test_name = "initialize_email_empty",
          .email = "",
          .blinded_message = {},    // not used
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
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
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(11005);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(13003);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(13001);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(13004);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(13006);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body = std::nullopt}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token = "";
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token =
                                           "verification_token";
                                       body.serialized_response = "";
                                       return body;
                                     }()}},
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
          .fail_encryption = true,
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token =
                                           "verification_token";
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
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
          .fail_encryption = false,
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token =
                                           "verification_token";
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
          .mojo_expected = mojom::RegisterInitializeResult::New(
              base::Base64Encode("verification_token"), "serialized_response"),
      });
  return kInitializeSuccess.get();
}

using RegisterInitializeTest =
    BraveAccountServiceTest<RegisterInitializeTestCase>;

}  // namespace

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
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::RegisterFinalizeResultPtr, mojom::RegisterErrorPtr>;

  static void Run(const RegisterFinalizeTestCase& test_case,
                  mojom::Authentication& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication.RegisterFinalize(test_case.encrypted_verification_token,
                                    test_case.serialized_record,
                                    std::move(callback));
  }

  std::string test_name;
  std::string encrypted_verification_token;
  std::string serialized_record;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const RegisterFinalizeTestCase* FinalizeEncryptedVerificationTokenEmpty() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kFinalizeEncryptedVerificationTokenEmpty({
          .test_name = "finalize_encrypted_verification_token_empty",
          .encrypted_verification_token = "",
          .serialized_record = {},  // not used
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
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
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
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
          .fail_encryption = {},  // not used
          .fail_decryption = true,
          .endpoint_response = {},  // not used
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
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14001);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14002);
                                   return body;
                                 }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
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
      .fail_encryption = {},  // not used
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_FORBIDDEN,
                             .body = base::unexpected([] {
                               PasswordFinalize::Response::ErrorBody body;
                               body.code = base::Value(0);
                               return body;
                             }())}},
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
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
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
      .fail_encryption = {},  // not used
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_TOO_EARLY,
                             .body = base::unexpected([] {
                               PasswordFinalize::Response::ErrorBody body;
                               body.code = base::Value(42);
                               return body;
                             }())}},
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
      .fail_encryption = {},  // not used
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 PasswordFinalize::Response::SuccessBody()}},
      .mojo_expected = mojom::RegisterFinalizeResult::New(),
  });
  return kFinalizeSuccess.get();
}

using RegisterFinalizeTest = BraveAccountServiceTest<RegisterFinalizeTestCase>;

}  // namespace

TEST_P(RegisterFinalizeTest, MapsEndpointExpectedToMojoExpected) {
  RunTestCase();

  if (const auto& test_case = CHECK_DEREF(this->GetParam());
      test_case.mojo_expected.has_value()) {
    EXPECT_EQ(pref_service_.GetString(prefs::kBraveAccountVerificationToken),
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

struct VerifyResultTestCase {
  using Endpoint = VerifyResult;
  using EndpointResponse = Endpoint::Response;

  static void Run(const VerifyResultTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  base::OneShotTimer& verify_result_timer) {
    pref_service.SetString(prefs::kBraveAccountVerificationToken,
                           test_case.encrypted_verification_token);

    task_environment.FastForwardBy(kVerifyResultPollInterval -
                                   base::Seconds(1));

    EXPECT_EQ(pref_service.GetString(prefs::kBraveAccountVerificationToken),
              test_case.expected_verification_token);
    EXPECT_EQ(pref_service.GetString(prefs::kBraveAccountAuthenticationToken),
              test_case.expected_authentication_token);
    if (test_case.expected_verify_result_timer_delay.is_zero()) {
      EXPECT_FALSE(verify_result_timer.IsRunning());
    } else {
      EXPECT_TRUE(verify_result_timer.IsRunning());
      EXPECT_EQ(verify_result_timer.GetCurrentDelay(),
                test_case.expected_verify_result_timer_delay);
    }
  }

  std::string test_name;
  std::string encrypted_verification_token;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  std::string expected_verification_token;
  std::string expected_authentication_token;
  base::TimeDelta expected_verify_result_timer_delay;
};

namespace {

const VerifyResultTestCase* VerifyResultVerificationTokenEmpty() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultVerificationTokenEmpty({
          .test_name = "verify_result_verification_token_empty",
          .encrypted_verification_token = "",
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .expected_verification_token = "",
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = {},
      });
  return kVerifyResultVerificationTokenEmpty.get();
}

const VerifyResultTestCase* VerifyResultVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultVerificationTokenFailedToDecrypt({
          .test_name = "verify_result_verification_token_failed_to_decrypt",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = {},  // not used
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = {},
      });
  return kVerifyResultVerificationTokenFailedToDecrypt.get();
}

const VerifyResultTestCase* VerifyResultSuccessAuthTokenNull() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultSuccessAuthTokenNull({
          .test_name = "verify_result_success_auth_token_null",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyResult::Response::SuccessBody body;
                                       body.auth_token = base::Value();
                                       return body;
                                     }()}},
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = kVerifyResultPollInterval,
      });
  return kVerifyResultSuccessAuthTokenNull.get();
}

const VerifyResultTestCase* VerifyResultSuccessAuthTokenEmpty() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultSuccessAuthTokenEmpty({
          .test_name = "verify_result_success_auth_token_empty",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyResult::Response::SuccessBody body;
                                       body.auth_token = base::Value("");
                                       return body;
                                     }()}},
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = kVerifyResultPollInterval,
      });
  return kVerifyResultSuccessAuthTokenEmpty.get();
}

const VerifyResultTestCase* VerifyResultSuccess() {
  static const base::NoDestructor<VerifyResultTestCase> kVerifyResultSuccess({
      .test_name = "verify_result_success",
      .encrypted_verification_token =
          base::Base64Encode("encrypted_verification_token"),
      .fail_encryption = {},  // not used
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyResult::Response::SuccessBody body;
                                   body.auth_token = base::Value("auth_token");
                                   return body;
                                 }()}},
      .expected_verification_token = "",
      .expected_authentication_token = base::Base64Encode("auth_token"),
      .expected_verify_result_timer_delay = {},
  });
  return kVerifyResultSuccess.get();
}

const VerifyResultTestCase*
VerifyResultSuccessAuthenticationTokenFailedToEncrypt() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultSuccessAuthenticationTokenFailedToEncrypt({
          .test_name =
              "verify_result_success_authentication_token_failed_to_encrypt",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyResult::Response::SuccessBody body;
                                       body.auth_token =
                                           base::Value("auth_token");
                                       return body;
                                     }()}},
          .expected_verification_token = "",
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = {},
      });
  return kVerifyResultSuccessAuthenticationTokenFailedToEncrypt.get();
}

const VerifyResultTestCase* VerifyResultBadRequest() {
  static const base::NoDestructor<VerifyResultTestCase> kVerifyResultBadRequest(
      {
          .test_name = "verify_result_bad_request",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = false,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyResult::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .expected_verification_token = "",
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = {},
      });
  return kVerifyResultBadRequest.get();
}

const VerifyResultTestCase* VerifyResultUnauthorized() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultUnauthorized({
          .test_name = "verify_result_unauthorized",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = false,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   VerifyResult::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .expected_verification_token = "",
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = {},
      });
  return kVerifyResultUnauthorized.get();
}

const VerifyResultTestCase* VerifyResultInternalServerError() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultInternalServerError({
          .test_name = "verify_result_internal_server_error",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   VerifyResult::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_verify_result_timer_delay = kVerifyResultPollInterval,
      });
  return kVerifyResultInternalServerError.get();
}

using ScheduleVerifyResultTest = BraveAccountServiceTest<VerifyResultTestCase>;

}  // namespace

TEST_P(ScheduleVerifyResultTest, HandlesVerifyResultOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    ScheduleVerifyResultTest,
    testing::Values(VerifyResultVerificationTokenEmpty(),
                    VerifyResultVerificationTokenFailedToDecrypt(),
                    VerifyResultSuccessAuthTokenNull(),
                    VerifyResultSuccessAuthTokenEmpty(),
                    VerifyResultSuccess(),
                    VerifyResultSuccessAuthenticationTokenFailedToEncrypt(),
                    VerifyResultBadRequest(),
                    VerifyResultUnauthorized(),
                    VerifyResultInternalServerError()),
    ScheduleVerifyResultTest::kNameGenerator);

struct CancelRegistrationTestCase {
  static void Run(const CancelRegistrationTestCase& test_case,
                  PrefService& pref_service,
                  mojom::Authentication& authentication) {
    pref_service.SetString(prefs::kBraveAccountVerificationToken,
                           test_case.encrypted_verification_token);
    authentication.CancelRegistration();
    EXPECT_EQ(pref_service.GetString(prefs::kBraveAccountVerificationToken),
              test_case.expected_verification_token);
  }

  std::string test_name;
  std::string encrypted_verification_token;
  std::string expected_verification_token;
};

namespace {

const CancelRegistrationTestCase* CancelRegistrationVerificationTokenEmpty() {
  static const base::NoDestructor<CancelRegistrationTestCase>
      kCancelRegistrationVerificationTokenEmpty({
          .test_name = "cancel_registration_verification_token_empty",
          .encrypted_verification_token = "",
          .expected_verification_token = "",
      });
  return kCancelRegistrationVerificationTokenEmpty.get();
}

const CancelRegistrationTestCase*
CancelRegistrationVerificationTokenNonEmpty() {
  static const base::NoDestructor<CancelRegistrationTestCase>
      kCancelRegistrationVerificationTokenNonEmpty({
          .test_name = "cancel_registration_verification_token_non_empty",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_verification_token = "",
      });
  return kCancelRegistrationVerificationTokenNonEmpty.get();
}

using CancelRegistrationTest =
    BraveAccountServiceTest<CancelRegistrationTestCase>;

}  // namespace

TEST_P(CancelRegistrationTest, HandlesCancelRegistrationOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    CancelRegistrationTest,
    testing::Values(CancelRegistrationVerificationTokenEmpty(),
                    CancelRegistrationVerificationTokenNonEmpty()),
    CancelRegistrationTest::kNameGenerator);

struct LogOutTestCase {
  static void Run(const LogOutTestCase& test_case,
                  PrefService& pref_service,
                  mojom::Authentication& authentication) {
    pref_service.SetString(prefs::kBraveAccountAuthenticationToken,
                           test_case.encrypted_authentication_token);
    authentication.LogOut();
    EXPECT_EQ(pref_service.GetString(prefs::kBraveAccountAuthenticationToken),
              test_case.expected_authentication_token);
  }

  std::string test_name;
  std::string encrypted_authentication_token;
  std::string expected_authentication_token;
};

namespace {

const LogOutTestCase* LogOutAuthenticationTokenEmpty() {
  static const base::NoDestructor<LogOutTestCase>
      kLogOutAuthenticationTokenEmpty({
          .test_name = "log_out_authentication_token_empty",
          .encrypted_authentication_token = "",
          .expected_authentication_token = "",
      });
  return kLogOutAuthenticationTokenEmpty.get();
}

const LogOutTestCase* LogOutAuthenticationTokenNonEmpty() {
  static const base::NoDestructor<LogOutTestCase>
      kLogOutAuthenticationTokenNonEmpty({
          .test_name = "log_out_authentication_token_non_empty",
          .encrypted_authentication_token =
              base::Base64Encode("authentication_token"),
          .expected_authentication_token = "",
      });
  return kLogOutAuthenticationTokenNonEmpty.get();
}

using LogOutTest = BraveAccountServiceTest<LogOutTestCase>;

}  // namespace

TEST_P(LogOutTest, HandlesLogOutOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(BraveAccountServiceTests,
                         LogOutTest,
                         testing::Values(LogOutAuthenticationTokenEmpty(),
                                         LogOutAuthenticationTokenNonEmpty()),
                         LogOutTest::kNameGenerator);

}  // namespace brave_account
