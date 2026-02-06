/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/functional/callback.h"
#include "base/json/values_util.h"
#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/endpoints/auth_validate.h"
#include "brave/components/brave_account/endpoints/login_finalize.h"
#include "brave/components/brave_account/endpoints/login_init.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/service_token.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/endpoints/verify_result.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::AuthValidate;
using endpoints::LoginFinalize;
using endpoints::LoginInit;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::ServiceToken;
using endpoints::VerifyResend;
using endpoints::VerifyResult;

struct RegisterInitializeTestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::RegisterInitializeResultPtr,
                                      mojom::RegisterErrorPtr>;

  static void Run(const RegisterInitializeTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
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

const RegisterInitializeTestCase* RegisterInitializeEmailEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeEmailEmpty({
          .test_name = "register_initialize_email_empty",
          .email = "",
          .blinded_message = {},    // not used
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kRegisterInitializeEmailEmpty.get();
}

const RegisterInitializeTestCase* RegisterInitializeBlindedMessageEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeBlindedMessageEmpty({
          .test_name = "register_initialize_blinded_message_empty",
          .email = "email",
          .blinded_message = "",
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kRegisterInitializeBlindedMessageEmpty.get();
}

const RegisterInitializeTestCase*
RegisterInitializeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeBodyMissingOrFailedToParse({
          .test_name = "register_initialize_body_missing_or_failed_to_parse",
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
  return kRegisterInitializeBodyMissingOrFailedToParse.get();
}

const RegisterInitializeTestCase* RegisterInitializeErrorCodeIsNull() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeErrorCodeIsNull({
          .test_name = "register_initialize_error_code_is_null",
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
  return kRegisterInitializeErrorCodeIsNull.get();
}

const RegisterInitializeTestCase* RegisterInitializeNewAccountEmailRequired() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeNewAccountEmailRequired({
          .test_name = "register_initialize_new_account_email_required",
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
  return kRegisterInitializeNewAccountEmailRequired.get();
}

const RegisterInitializeTestCase* RegisterInitializeIntentNotAllowed() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeIntentNotAllowed({
          .test_name = "register_initialize_intent_not_allowed",
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
  return kRegisterInitializeIntentNotAllowed.get();
}

const RegisterInitializeTestCase* RegisterInitializeTooManyVerifications() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeTooManyVerifications({
          .test_name = "register_initialize_too_many_verifications",
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
  return kRegisterInitializeTooManyVerifications.get();
}

const RegisterInitializeTestCase* RegisterInitializeAccountExists() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeAccountExists({
          .test_name = "register_initialize_account_exists",
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
  return kRegisterInitializeAccountExists.get();
}

const RegisterInitializeTestCase* RegisterInitializeEmailDomainNotSupported() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeEmailDomainNotSupported({
          .test_name = "register_initialize_email_domain_not_supported",
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
  return kRegisterInitializeEmailDomainNotSupported.get();
}

const RegisterInitializeTestCase* RegisterInitializeUnauthorized() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeUnauthorized({
          .test_name = "register_initialize_unauthorized",
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
  return kRegisterInitializeUnauthorized.get();
}

const RegisterInitializeTestCase* RegisterInitializeServerError() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeServerError({
          .test_name = "register_initialize_server_error",
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
  return kRegisterInitializeServerError.get();
}

const RegisterInitializeTestCase* RegisterInitializeUnknown() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeUnknown({
          .test_name = "register_initialize_unknown",
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
  return kRegisterInitializeUnknown.get();
}

const RegisterInitializeTestCase* RegisterInitializeVerificationTokenEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeVerificationTokenEmpty({
          .test_name = "register_initialize_verification_token_empty",
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
  return kRegisterInitializeVerificationTokenEmpty.get();
}

const RegisterInitializeTestCase* RegisterInitializeSerializedResponseEmpty() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeSerializedResponseEmpty({
          .test_name = "register_initialize_serialized_response_empty",
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
  return kRegisterInitializeSerializedResponseEmpty.get();
}

const RegisterInitializeTestCase*
RegisterInitializeVerificationTokenFailedToEncrypt() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeVerificationTokenFailedToEncrypt({
          .test_name =
              "register_initialize_verification_token_failed_to_encrypt",
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
  return kRegisterInitializeVerificationTokenFailedToEncrypt.get();
}

const RegisterInitializeTestCase* RegisterInitializeSuccess() {
  static const base::NoDestructor<RegisterInitializeTestCase>
      kRegisterInitializeSuccess({
          .test_name = "register_initialize_success",
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
  return kRegisterInitializeSuccess.get();
}

using BraveAccountServiceRegisterInitializeTest =
    BraveAccountServiceTest<RegisterInitializeTestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterInitializeTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceRegisterInitializeTest,
    testing::Values(RegisterInitializeEmailEmpty(),
                    RegisterInitializeBlindedMessageEmpty(),
                    RegisterInitializeBodyMissingOrFailedToParse(),
                    RegisterInitializeErrorCodeIsNull(),
                    RegisterInitializeNewAccountEmailRequired(),
                    RegisterInitializeIntentNotAllowed(),
                    RegisterInitializeTooManyVerifications(),
                    RegisterInitializeAccountExists(),
                    RegisterInitializeEmailDomainNotSupported(),
                    RegisterInitializeUnauthorized(),
                    RegisterInitializeServerError(),
                    RegisterInitializeUnknown(),
                    RegisterInitializeVerificationTokenEmpty(),
                    RegisterInitializeSerializedResponseEmpty(),
                    RegisterInitializeVerificationTokenFailedToEncrypt(),
                    RegisterInitializeSuccess()),
    BraveAccountServiceRegisterInitializeTest::kNameGenerator);

struct RegisterFinalizeTestCase {
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::RegisterFinalizeResultPtr, mojom::RegisterErrorPtr>;

  static void Run(const RegisterFinalizeTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
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

const RegisterFinalizeTestCase*
RegisterFinalizeEncryptedVerificationTokenEmpty() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeEncryptedVerificationTokenEmpty({
          .test_name = "register_finalize_encrypted_verification_token_empty",
          .encrypted_verification_token = "",
          .serialized_record = {},  // not used
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kRegisterFinalizeEncryptedVerificationTokenEmpty.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeSerializedRecordEmpty() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeSerializedRecordEmpty({
          .test_name = "register_finalize_serialized_record_empty",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "",
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(mojom::RegisterError::New()),
      });
  return kRegisterFinalizeSerializedRecordEmpty.get();
}

const RegisterFinalizeTestCase*
RegisterFinalizeVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeVerificationTokenFailedToDecrypt({
          .test_name = "register_finalize_verification_token_failed_to_decrypt",
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
  return kRegisterFinalizeVerificationTokenFailedToDecrypt.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeBodyMissingOrFailedToParse({
          .test_name = "register_finalize_body_missing_or_failed_to_parse",
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
  return kRegisterFinalizeBodyMissingOrFailedToParse.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeErrorCodeIsNull() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeErrorCodeIsNull({
          .test_name = "register_finalize_error_code_is_null",
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
  return kRegisterFinalizeErrorCodeIsNull.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeInterimPasswordStateNotFound() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeInterimPasswordStateNotFound({
          .test_name = "register_finalize_interim_password_state_not_found",
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
  return kRegisterFinalizeInterimPasswordStateNotFound.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeInterimPasswordStateExpired() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeInterimPasswordStateExpired({
          .test_name = "register_finalize_interim_password_state_expired",
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
  return kRegisterFinalizeInterimPasswordStateExpired.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeUnauthorized() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeUnauthorized({
          .test_name = "register_finalize_unauthorized",
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
  return kRegisterFinalizeUnauthorized.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeForbidden() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeForbidden({
          .test_name = "register_finalize_forbidden",
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
  return kRegisterFinalizeForbidden.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeServerError() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeServerError({
          .test_name = "register_finalize_server_error",
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
  return kRegisterFinalizeServerError.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeUnknown() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeUnknown({
          .test_name = "register_finalize_unknown",
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
  return kRegisterFinalizeUnknown.get();
}

const RegisterFinalizeTestCase* RegisterFinalizeSuccess() {
  static const base::NoDestructor<RegisterFinalizeTestCase>
      kRegisterFinalizeSuccess({
          .test_name = "register_finalize_success",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .serialized_record = "serialized_record",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body = PasswordFinalize::Response::SuccessBody()}},
          .mojo_expected = mojom::RegisterFinalizeResult::New(),
      });
  return kRegisterFinalizeSuccess.get();
}

using BraveAccountServiceRegisterFinalizeTest =
    BraveAccountServiceTest<RegisterFinalizeTestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterFinalizeTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();

  if (const auto& test_case = CHECK_DEREF(this->GetParam());
      test_case.mojo_expected.has_value()) {
    EXPECT_EQ(pref_service_.GetString(prefs::kBraveAccountVerificationToken),
              test_case.encrypted_verification_token);
  }
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceRegisterFinalizeTest,
    testing::Values(RegisterFinalizeEncryptedVerificationTokenEmpty(),
                    RegisterFinalizeSerializedRecordEmpty(),
                    RegisterFinalizeVerificationTokenFailedToDecrypt(),
                    RegisterFinalizeBodyMissingOrFailedToParse(),
                    RegisterFinalizeErrorCodeIsNull(),
                    RegisterFinalizeInterimPasswordStateNotFound(),
                    RegisterFinalizeInterimPasswordStateExpired(),
                    RegisterFinalizeUnauthorized(),
                    RegisterFinalizeForbidden(),
                    RegisterFinalizeServerError(),
                    RegisterFinalizeUnknown(),
                    RegisterFinalizeSuccess()),
    BraveAccountServiceRegisterFinalizeTest::kNameGenerator);

struct ResendConfirmationEmailTestCase {
  using Endpoint = VerifyResend;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResendConfirmationEmailResultPtr,
                                      mojom::ResendConfirmationEmailErrorPtr>;

  static void Run(const ResendConfirmationEmailTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojom::Authentication& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    if (!test_case.encrypted_verification_token.empty()) {
      pref_service.SetString(prefs::kBraveAccountVerificationToken,
                             test_case.encrypted_verification_token);
    }

    authentication.ResendConfirmationEmail(std::move(callback));
  }

  std::string test_name;
  std::string encrypted_verification_token;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ResendConfirmationEmailTestCase*
ResendConfirmationEmailVerificationTokenEmpty() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailVerificationTokenEmpty({
          .test_name = "resend_confirmation_email_verification_token_empty",
          .encrypted_verification_token = "",
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  std::nullopt, mojom::ResendConfirmationEmailErrorCode::
                                    kUserNotInTheVerificationState)),
      });
  return kResendConfirmationEmailVerificationTokenEmpty.get();
}

const ResendConfirmationEmailTestCase*
ResendConfirmationEmailVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailVerificationTokenFailedToDecrypt({
          .test_name =
              "resend_confirmation_email_verification_token_failed_to_decrypt",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  std::nullopt, mojom::ResendConfirmationEmailErrorCode::
                                    kVerificationTokenDecryptionFailed)),
      });
  return kResendConfirmationEmailVerificationTokenFailedToDecrypt.get();
}

const ResendConfirmationEmailTestCase* ResendConfirmationEmailNetworkError() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailNetworkError({
          .test_name = "resend_confirmation_email_network_error",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::ERR_CONNECTION_REFUSED,
                                 .status_code = std::nullopt,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  net::ERR_CONNECTION_REFUSED, std::nullopt)),
      });
  return kResendConfirmationEmailNetworkError.get();
}

const ResendConfirmationEmailTestCase*
ResendConfirmationEmailBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailBodyMissingOrFailedToParse({
          .test_name =
              "resend_confirmation_email_body_missing_or_failed_to_parse",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  net::HTTP_INTERNAL_SERVER_ERROR, std::nullopt)),
      });
  return kResendConfirmationEmailBodyMissingOrFailedToParse.get();
}

const ResendConfirmationEmailTestCase*
ResendConfirmationEmailBadRequestWithNullErrorCode() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailBadRequestWithNullErrorCode({
          .test_name =
              "resend_confirmation_email_bad_request_with_null_error_code",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  net::HTTP_BAD_REQUEST, std::nullopt)),
      });
  return kResendConfirmationEmailBadRequestWithNullErrorCode.get();
}

const ResendConfirmationEmailTestCase*
ResendConfirmationEmailMaximumEmailSendAttemptsExceeded() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailMaximumEmailSendAttemptsExceeded({
          .test_name =
              "resend_confirmation_email_maximum_email_send_attempts_exceeded",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value(13008);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  net::HTTP_BAD_REQUEST,
                  mojom::ResendConfirmationEmailErrorCode::
                      kMaximumEmailSendAttemptsExceeded)),
      });
  return kResendConfirmationEmailMaximumEmailSendAttemptsExceeded.get();
}

const ResendConfirmationEmailTestCase*
ResendConfirmationEmailEmailAlreadyVerified() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailEmailAlreadyVerified({
          .test_name = "resend_confirmation_email_email_already_verified",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value(13009);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  net::HTTP_BAD_REQUEST,
                  mojom::ResendConfirmationEmailErrorCode::
                      kEmailAlreadyVerified)),
      });
  return kResendConfirmationEmailEmailAlreadyVerified.get();
}

const ResendConfirmationEmailTestCase* ResendConfirmationEmailServerError() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailServerError({
          .test_name = "resend_confirmation_email_server_error",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  net::HTTP_INTERNAL_SERVER_ERROR, std::nullopt)),
      });
  return kResendConfirmationEmailServerError.get();
}

const ResendConfirmationEmailTestCase* ResendConfirmationEmailUnknown() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailUnknown({
          .test_name = "resend_confirmation_email_unknown",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResendConfirmationEmailError::New(
                  net::HTTP_TOO_EARLY, std::nullopt)),
      });
  return kResendConfirmationEmailUnknown.get();
}

const ResendConfirmationEmailTestCase* ResendConfirmationEmailSuccess() {
  static const base::NoDestructor<ResendConfirmationEmailTestCase>
      kResendConfirmationEmailSuccess({
          .test_name = "resend_confirmation_email_success",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NO_CONTENT,
                                 .body =
                                     VerifyResend::Response::SuccessBody()}},
          .mojo_expected = mojom::ResendConfirmationEmailResult::New(),
      });
  return kResendConfirmationEmailSuccess.get();
}

using BraveAccountServiceResendConfirmationEmailTest =
    BraveAccountServiceTest<ResendConfirmationEmailTestCase>;

}  // namespace

TEST_P(BraveAccountServiceResendConfirmationEmailTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResendConfirmationEmailTest,
    testing::Values(ResendConfirmationEmailVerificationTokenEmpty(),
                    ResendConfirmationEmailVerificationTokenFailedToDecrypt(),
                    ResendConfirmationEmailNetworkError(),
                    ResendConfirmationEmailBodyMissingOrFailedToParse(),
                    ResendConfirmationEmailBadRequestWithNullErrorCode(),
                    ResendConfirmationEmailMaximumEmailSendAttemptsExceeded(),
                    ResendConfirmationEmailEmailAlreadyVerified(),
                    ResendConfirmationEmailServerError(),
                    ResendConfirmationEmailUnknown(),
                    ResendConfirmationEmailSuccess()),
    BraveAccountServiceResendConfirmationEmailTest::kNameGenerator);

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
    EXPECT_EQ(pref_service.GetString(prefs::kBraveAccountEmailAddress),
              test_case.expected_email);
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
  std::string expected_email;
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
          .expected_email = "",
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
          .expected_email = "",
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
                                       body.email = std::nullopt;
                                       return body;
                                     }()}},
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_email = "",
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
                                       body.email = "email";
                                       return body;
                                     }()}},
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_email = "",
          .expected_verify_result_timer_delay = kVerifyResultPollInterval,
      });
  return kVerifyResultSuccessAuthTokenEmpty.get();
}

const VerifyResultTestCase* VerifyResultSuccessEmailNotPresent() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultSuccessEmailNotPresent({
          .test_name = "verify_result_success_email_not_present",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyResult::Response::SuccessBody body;
                                       body.auth_token =
                                           base::Value("auth_token");
                                       body.email = std::nullopt;
                                       return body;
                                     }()}},
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_email = "",
          .expected_verify_result_timer_delay = kVerifyResultPollInterval,
      });
  return kVerifyResultSuccessEmailNotPresent.get();
}

const VerifyResultTestCase* VerifyResultSuccessEmailEmpty() {
  static const base::NoDestructor<VerifyResultTestCase>
      kVerifyResultSuccessEmailEmpty({
          .test_name = "verify_result_success_email_empty",
          .encrypted_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyResult::Response::SuccessBody body;
                                       body.auth_token =
                                           base::Value("auth_token");
                                       body.email = "";
                                       return body;
                                     }()}},
          .expected_verification_token =
              base::Base64Encode("encrypted_verification_token"),
          .expected_authentication_token = "",
          .expected_email = "",
          .expected_verify_result_timer_delay = kVerifyResultPollInterval,
      });
  return kVerifyResultSuccessEmailEmpty.get();
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
                                   body.email = "email";
                                   return body;
                                 }()}},
      .expected_verification_token = "",
      .expected_authentication_token = base::Base64Encode("auth_token"),
      .expected_email = "email",
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
                                       body.email = "email";
                                       return body;
                                     }()}},
          .expected_verification_token = "",
          .expected_authentication_token = "",
          .expected_email = "",
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
          .expected_email = "",
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
          .expected_email = "",
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
          .expected_email = "",
          .expected_verify_result_timer_delay = kVerifyResultPollInterval,
      });
  return kVerifyResultInternalServerError.get();
}

using BraveAccountServiceScheduleVerifyResultTest =
    BraveAccountServiceTest<VerifyResultTestCase>;

}  // namespace

TEST_P(BraveAccountServiceScheduleVerifyResultTest,
       HandlesVerifyResultOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceScheduleVerifyResultTest,
    testing::Values(VerifyResultVerificationTokenEmpty(),
                    VerifyResultVerificationTokenFailedToDecrypt(),
                    VerifyResultSuccessAuthTokenNull(),
                    VerifyResultSuccessAuthTokenEmpty(),
                    VerifyResultSuccessEmailNotPresent(),
                    VerifyResultSuccessEmailEmpty(),
                    VerifyResultSuccess(),
                    VerifyResultSuccessAuthenticationTokenFailedToEncrypt(),
                    VerifyResultBadRequest(),
                    VerifyResultUnauthorized(),
                    VerifyResultInternalServerError()),
    BraveAccountServiceScheduleVerifyResultTest::kNameGenerator);

struct AuthValidateTestCase {
  using Endpoint = AuthValidate;
  using EndpointResponse = Endpoint::Response;

  static void Run(const AuthValidateTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  base::OneShotTimer& auth_validate_timer) {
    pref_service.SetString(prefs::kBraveAccountAuthenticationToken,
                           test_case.encrypted_authentication_token);

    task_environment.FastForwardBy(kAuthValidatePollInterval -
                                   base::Seconds(1));

    EXPECT_EQ(pref_service.GetString(prefs::kBraveAccountEmailAddress),
              test_case.expected_email);
    EXPECT_EQ(pref_service.GetString(prefs::kBraveAccountAuthenticationToken),
              test_case.expected_authentication_token);
    if (test_case.expected_auth_validate_timer_delay.is_zero()) {
      EXPECT_FALSE(auth_validate_timer.IsRunning());
    } else {
      EXPECT_TRUE(auth_validate_timer.IsRunning());
      EXPECT_EQ(auth_validate_timer.GetCurrentDelay(),
                test_case.expected_auth_validate_timer_delay);
    }
  }

  std::string test_name;
  std::string encrypted_authentication_token;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  std::string expected_email;
  std::string expected_authentication_token;
  base::TimeDelta expected_auth_validate_timer_delay;
};

namespace {

const AuthValidateTestCase* AuthValidateAuthenticationTokenEmpty() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateAuthenticationTokenEmpty({
          .test_name = "auth_validate_authentication_token_empty",
          .encrypted_authentication_token = "",
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .expected_email = "",
          .expected_authentication_token = "",
          .expected_auth_validate_timer_delay = {},
      });
  return kAuthValidateAuthenticationTokenEmpty.get();
}

const AuthValidateTestCase* AuthValidateAuthenticationTokenFailedToDecrypt() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateAuthenticationTokenFailedToDecrypt({
          .test_name = "auth_validate_authentication_token_failed_to_decrypt",
          .encrypted_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .expected_email = "",
          .expected_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .expected_auth_validate_timer_delay = {},
      });
  return kAuthValidateAuthenticationTokenFailedToDecrypt.get();
}

const AuthValidateTestCase* AuthValidateSuccessNoBody() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateSuccessNoBody({
          .test_name = "auth_validate_success_no_body",
          .encrypted_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body = {}}},
          .expected_email = "",
          .expected_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
      });
  return kAuthValidateSuccessNoBody.get();
}

const AuthValidateTestCase* AuthValidateSuccessEmailEmpty() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateSuccessEmailEmpty({
          .test_name = "auth_validate_success_email_empty",
          .encrypted_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       AuthValidate::Response::SuccessBody body;
                                       body.email = "";
                                       return body;
                                     }()}},
          .expected_email = "",
          .expected_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
      });
  return kAuthValidateSuccessEmailEmpty.get();
}

const AuthValidateTestCase* AuthValidateSuccess() {
  static const base::NoDestructor<AuthValidateTestCase> kAuthValidateSuccess({
      .test_name = "auth_validate_success",
      .encrypted_authentication_token =
          base::Base64Encode("encrypted_authentication_token"),
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   AuthValidate::Response::SuccessBody body;
                                   body.email = "email";
                                   return body;
                                 }()}},
      .expected_email = "email",
      .expected_authentication_token =
          base::Base64Encode("encrypted_authentication_token"),
      .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
  });
  return kAuthValidateSuccess.get();
}

const AuthValidateTestCase* AuthValidateUnauthorized() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateUnauthorized({
          .test_name = "auth_validate_unauthorized",
          .encrypted_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
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
          .expected_auth_validate_timer_delay = {},
      });
  return kAuthValidateUnauthorized.get();
}

const AuthValidateTestCase* AuthValidateForbidden() {
  static const base::NoDestructor<AuthValidateTestCase> kAuthValidateForbidden({
      .test_name = "auth_validate_forbidden",
      .encrypted_authentication_token =
          base::Base64Encode("encrypted_authentication_token"),
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_FORBIDDEN,
                             .body = base::unexpected([] {
                               AuthValidate::Response::ErrorBody body;
                               body.code = base::Value(14007);
                               return body;
                             }())}},
      .expected_email = "",
      .expected_authentication_token = "",
      .expected_auth_validate_timer_delay = {},
  });
  return kAuthValidateForbidden.get();
}

const AuthValidateTestCase* AuthValidateInternalServerError() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateInternalServerError({
          .test_name = "auth_validate_internal_server_error",
          .encrypted_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   AuthValidate::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token =
              base::Base64Encode("encrypted_authentication_token"),
          .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
      });
  return kAuthValidateInternalServerError.get();
}

using BraveAccountServiceScheduleAuthValidateTest =
    BraveAccountServiceTest<AuthValidateTestCase>;

}  // namespace

TEST_P(BraveAccountServiceScheduleAuthValidateTest,
       HandlesAuthValidateOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceScheduleAuthValidateTest,
    testing::Values(AuthValidateAuthenticationTokenEmpty(),
                    AuthValidateAuthenticationTokenFailedToDecrypt(),
                    AuthValidateSuccessNoBody(),
                    AuthValidateSuccessEmailEmpty(),
                    AuthValidateSuccess(),
                    AuthValidateUnauthorized(),
                    AuthValidateForbidden(),
                    AuthValidateInternalServerError()),
    BraveAccountServiceScheduleAuthValidateTest::kNameGenerator);

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

using BraveAccountServiceCancelRegistrationTest =
    BraveAccountServiceTest<CancelRegistrationTestCase>;

}  // namespace

TEST_P(BraveAccountServiceCancelRegistrationTest,
       HandlesCancelRegistrationOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceCancelRegistrationTest,
    testing::Values(CancelRegistrationVerificationTokenEmpty(),
                    CancelRegistrationVerificationTokenNonEmpty()),
    BraveAccountServiceCancelRegistrationTest::kNameGenerator);

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

using BraveAccountServiceLogOutTest = BraveAccountServiceTest<LogOutTestCase>;

}  // namespace

TEST_P(BraveAccountServiceLogOutTest, HandlesLogOutOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(BraveAccountServiceTests,
                         BraveAccountServiceLogOutTest,
                         testing::Values(LogOutAuthenticationTokenEmpty(),
                                         LogOutAuthenticationTokenNonEmpty()),
                         BraveAccountServiceLogOutTest::kNameGenerator);

struct LoginInitializeTestCase {
  using Endpoint = LoginInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::LoginInitializeResultPtr, mojom::LoginErrorPtr>;

  static void Run(const LoginInitializeTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojom::Authentication& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication.LoginInitialize(test_case.email, test_case.serialized_ke1,
                                   std::move(callback));
  }

  std::string test_name;
  std::string email;
  std::string serialized_ke1;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const LoginInitializeTestCase* LoginInitializeEmailEmpty() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeEmailEmpty({
          .test_name = "login_initialize_email_empty",
          .email = "",
          .serialized_ke1 = {},     // not used
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(mojom::LoginError::New()),
      });
  return kLoginInitializeEmailEmpty.get();
}

const LoginInitializeTestCase* LoginInitializeSerializedKe1Empty() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeSerializedKe1Empty({
          .test_name = "login_initialize_serialized_ke1_empty",
          .email = "email",
          .serialized_ke1 = "",
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(mojom::LoginError::New()),
      });
  return kLoginInitializeSerializedKe1Empty.get();
}

const LoginInitializeTestCase* LoginInitializeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeBodyMissingOrFailedToParse({
          .test_name = "login_initialize_body_missing_or_failed_to_parse",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_INTERNAL_SERVER_ERROR, std::nullopt)),
      });
  return kLoginInitializeBodyMissingOrFailedToParse.get();
}

const LoginInitializeTestCase* LoginInitializeErrorCodeIsNull() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeErrorCodeIsNull({
          .test_name = "login_initialize_error_code_is_null",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_BAD_REQUEST, std::nullopt)),
      });
  return kLoginInitializeErrorCodeIsNull.get();
}

const LoginInitializeTestCase* LoginInitializeEmailNotVerified() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeEmailNotVerified({
          .test_name = "login_initialize_email_not_verified",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(11003);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_UNAUTHORIZED,
                                     mojom::LoginErrorCode::kEmailNotVerified)),
      });
  return kLoginInitializeEmailNotVerified.get();
}

const LoginInitializeTestCase* LoginInitializeIncorrectCredentials() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeIncorrectCredentials({
          .test_name = "login_initialize_incorrect_credentials",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(14004);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::LoginErrorCode::kIncorrectCredentials)),
      });
  return kLoginInitializeIncorrectCredentials.get();
}

const LoginInitializeTestCase* LoginInitializeIncorrectEmail() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeIncorrectEmail({
          .test_name = "login_initialize_incorrect_email",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(14005);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED, mojom::LoginErrorCode::kIncorrectEmail)),
      });
  return kLoginInitializeIncorrectEmail.get();
}

const LoginInitializeTestCase* LoginInitializeIncorrectPassword() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeIncorrectPassword({
          .test_name = "login_initialize_incorrect_password",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(14006);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::LoginErrorCode::kIncorrectPassword)),
      });
  return kLoginInitializeIncorrectPassword.get();
}

const LoginInitializeTestCase* LoginInitializeServerError() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeServerError({
          .test_name = "login_initialize_server_error",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_INTERNAL_SERVER_ERROR,
                                     mojom::LoginErrorCode::kMiscServerError)),
      });
  return kLoginInitializeServerError.get();
}

const LoginInitializeTestCase* LoginInitializeUnknown() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeUnknown({
          .test_name = "login_initialize_unknown",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_TOO_EARLY, std::nullopt)),
      });
  return kLoginInitializeUnknown.get();
}

const LoginInitializeTestCase* LoginInitializeLoginTokenEmpty() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeLoginTokenEmpty({
          .test_name = "login_initialize_login_token_empty",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginInit::Response::SuccessBody body;
                                       body.login_token = "";
                                       body.serialized_ke2 = "serialized_ke2";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_OK, std::nullopt)),
      });
  return kLoginInitializeLoginTokenEmpty.get();
}

const LoginInitializeTestCase* LoginInitializeSerializedKe2Empty() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeSerializedKe2Empty({
          .test_name = "login_initialize_serialized_ke2_empty",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginInit::Response::SuccessBody body;
                                       body.login_token = "login_token";
                                       body.serialized_ke2 = "";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_OK, std::nullopt)),
      });
  return kLoginInitializeSerializedKe2Empty.get();
}

const LoginInitializeTestCase* LoginInitializeLoginTokenFailedToEncrypt() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeLoginTokenFailedToEncrypt({
          .test_name = "login_initialize_login_token_failed_to_encrypt",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = true,
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginInit::Response::SuccessBody body;
                                       body.login_token = "login_token";
                                       body.serialized_ke2 = "serialized_ke2";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              std::nullopt,
              mojom::LoginErrorCode::kLoginTokenEncryptionFailed)),
      });
  return kLoginInitializeLoginTokenFailedToEncrypt.get();
}

const LoginInitializeTestCase* LoginInitializeSuccess() {
  static const base::NoDestructor<LoginInitializeTestCase>
      kLoginInitializeSuccess({
          .test_name = "login_initialize_success",
          .email = "email",
          .serialized_ke1 = "serialized_ke1",
          .fail_encryption = false,
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginInit::Response::SuccessBody body;
                                       body.login_token = "login_token";
                                       body.serialized_ke2 = "serialized_ke2";
                                       return body;
                                     }()}},
          .mojo_expected = mojom::LoginInitializeResult::New(
              base::Base64Encode("login_token"), "serialized_ke2"),
      });
  return kLoginInitializeSuccess.get();
}

using BraveAccountServiceLoginInitializeTest =
    BraveAccountServiceTest<LoginInitializeTestCase>;

}  // namespace

TEST_P(BraveAccountServiceLoginInitializeTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceLoginInitializeTest,
    testing::Values(LoginInitializeEmailEmpty(),
                    LoginInitializeSerializedKe1Empty(),
                    LoginInitializeBodyMissingOrFailedToParse(),
                    LoginInitializeErrorCodeIsNull(),
                    LoginInitializeEmailNotVerified(),
                    LoginInitializeIncorrectCredentials(),
                    LoginInitializeIncorrectEmail(),
                    LoginInitializeIncorrectPassword(),
                    LoginInitializeServerError(),
                    LoginInitializeUnknown(),
                    LoginInitializeLoginTokenEmpty(),
                    LoginInitializeSerializedKe2Empty(),
                    LoginInitializeLoginTokenFailedToEncrypt(),
                    LoginInitializeSuccess()),
    BraveAccountServiceLoginInitializeTest::kNameGenerator);

struct LoginFinalizeTestCase {
  using Endpoint = LoginFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::LoginFinalizeResultPtr, mojom::LoginErrorPtr>;

  static void Run(const LoginFinalizeTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojom::Authentication& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication.LoginFinalize(
        test_case.encrypted_login_token, test_case.client_mac,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, std::string expected_email,
               std::string expected_authentication_token) {
              EXPECT_EQ(
                  pref_service->GetString(prefs::kBraveAccountEmailAddress),
                  expected_email);
              EXPECT_EQ(pref_service->GetString(
                            prefs::kBraveAccountAuthenticationToken),
                        expected_authentication_token);
            },
            base::Unretained(&pref_service), test_case.expected_email,
            test_case.expected_authentication_token)));
  }

  std::string test_name;
  std::string encrypted_login_token;
  std::string client_mac;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  std::string expected_email;
  std::string expected_authentication_token;
  MojoExpected mojo_expected;
};

namespace {

const LoginFinalizeTestCase* LoginFinalizeEncryptedLoginTokenEmpty() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeEncryptedLoginTokenEmpty({
          .test_name = "login_finalize_encrypted_login_token_empty",
          .encrypted_login_token = "",
          .client_mac = {},         // not used
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New()),
      });
  return kLoginFinalizeEncryptedLoginTokenEmpty.get();
}

const LoginFinalizeTestCase* LoginFinalizeClientMacEmpty() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeClientMacEmpty({
          .test_name = "login_finalize_client_mac_empty",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "",
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New()),
      });
  return kLoginFinalizeClientMacEmpty.get();
}

const LoginFinalizeTestCase* LoginFinalizeLoginTokenFailedToDecrypt() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeLoginTokenFailedToDecrypt({
          .test_name = "login_finalize_login_token_failed_to_decrypt",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              std::nullopt,
              mojom::LoginErrorCode::kLoginTokenDecryptionFailed)),
      });
  return kLoginFinalizeLoginTokenFailedToDecrypt.get();
}

const LoginFinalizeTestCase* LoginFinalizeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeBodyMissingOrFailedToParse({
          .test_name = "login_finalize_body_missing_or_failed_to_parse",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_INTERNAL_SERVER_ERROR, std::nullopt)),
      });
  return kLoginFinalizeBodyMissingOrFailedToParse.get();
}

const LoginFinalizeTestCase* LoginFinalizeErrorCodeIsNull() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeErrorCodeIsNull({
          .test_name = "login_finalize_error_code_is_null",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_BAD_REQUEST, std::nullopt)),
      });
  return kLoginFinalizeErrorCodeIsNull.get();
}

const LoginFinalizeTestCase* LoginFinalizeInterimPasswordStateMismatch() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeInterimPasswordStateMismatch({
          .test_name = "login_finalize_interim_password_state_mismatch",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14009);
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_BAD_REQUEST,
              mojom::LoginErrorCode::kInterimPasswordStateMismatch)),
      });
  return kLoginFinalizeInterimPasswordStateMismatch.get();
}

const LoginFinalizeTestCase* LoginFinalizeInterimPasswordStateNotFound() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeInterimPasswordStateNotFound({
          .test_name = "login_finalize_interim_password_state_not_found",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14001);
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::LoginErrorCode::kInterimPasswordStateNotFound)),
      });
  return kLoginFinalizeInterimPasswordStateNotFound.get();
}

const LoginFinalizeTestCase* LoginFinalizeInterimPasswordStateHasExpired() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeInterimPasswordStateHasExpired({
          .test_name = "login_finalize_interim_password_state_has_expired",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14002);
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::LoginErrorCode::kInterimPasswordStateHasExpired)),
      });
  return kLoginFinalizeInterimPasswordStateHasExpired.get();
}

const LoginFinalizeTestCase* LoginFinalizeIncorrectCredentials() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeIncorrectCredentials({
          .test_name = "login_finalize_incorrect_credentials",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14004);
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::LoginErrorCode::kIncorrectCredentials)),
      });
  return kLoginFinalizeIncorrectCredentials.get();
}

const LoginFinalizeTestCase* LoginFinalizeIncorrectEmail() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeIncorrectEmail({
          .test_name = "login_finalize_incorrect_email",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14005);
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED, mojom::LoginErrorCode::kIncorrectEmail)),
      });
  return kLoginFinalizeIncorrectEmail.get();
}

const LoginFinalizeTestCase* LoginFinalizeIncorrectPassword() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeIncorrectPassword({
          .test_name = "login_finalize_incorrect_password",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14006);
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::LoginErrorCode::kIncorrectPassword)),
      });
  return kLoginFinalizeIncorrectPassword.get();
}

const LoginFinalizeTestCase* LoginFinalizeServerError() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeServerError({
          .test_name = "login_finalize_server_error",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_INTERNAL_SERVER_ERROR,
                                     mojom::LoginErrorCode::kMiscServerError)),
      });
  return kLoginFinalizeServerError.get();
}

const LoginFinalizeTestCase* LoginFinalizeUnknown() {
  static const base::NoDestructor<LoginFinalizeTestCase> kLoginFinalizeUnknown({
      .test_name = "login_finalize_unknown",
      .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
      .client_mac = "client_mac",
      .fail_encryption = {},  // not used
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_TOO_EARLY,
                             .body = base::unexpected([] {
                               LoginFinalize::Response::ErrorBody body;
                               body.code = base::Value(42);
                               return body;
                             }())}},
      .expected_email = "",
      .expected_authentication_token = "",
      .mojo_expected = base::unexpected(
          mojom::LoginError::New(net::HTTP_TOO_EARLY, std::nullopt)),
  });
  return kLoginFinalizeUnknown.get();
}

const LoginFinalizeTestCase* LoginFinalizeAuthTokenEmpty() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeAuthTokenEmpty({
          .test_name = "login_finalize_auth_token_empty",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = "";
                                       body.email = "email";
                                       return body;
                                     }()}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_OK, std::nullopt)),
      });
  return kLoginFinalizeAuthTokenEmpty.get();
}

const LoginFinalizeTestCase* LoginFinalizeEmailEmpty() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeEmailEmpty({
          .test_name = "login_finalize_email_empty",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = "auth_token";
                                       body.email = "";
                                       return body;
                                     }()}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(
              mojom::LoginError::New(net::HTTP_OK, std::nullopt)),
      });
  return kLoginFinalizeEmailEmpty.get();
}

const LoginFinalizeTestCase* LoginFinalizeAuthenticationTokenFailedToEncrypt() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kLoginFinalizeAuthenticationTokenFailedToEncrypt({
          .test_name = "login_finalize_authentication_token_failed_to_encrypt",
          .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
          .client_mac = "client_mac",
          .fail_encryption = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = "auth_token";
                                       body.email = "email";
                                       return body;
                                     }()}},
          .expected_email = "",
          .expected_authentication_token = "",
          .mojo_expected = base::unexpected(mojom::LoginError::New(
              std::nullopt,
              mojom::LoginErrorCode::kAuthenticationTokenEncryptionFailed)),
      });
  return kLoginFinalizeAuthenticationTokenFailedToEncrypt.get();
}

const LoginFinalizeTestCase* LoginFinalizeSuccess() {
  static const base::NoDestructor<LoginFinalizeTestCase> kLoginFinalizeSuccess({
      .test_name = "login_finalize_success",
      .encrypted_login_token = base::Base64Encode("encrypted_login_token"),
      .client_mac = "client_mac",
      .fail_encryption = false,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   LoginFinalize::Response::SuccessBody body;
                                   body.auth_token = "auth_token";
                                   body.email = "email";
                                   return body;
                                 }()}},
      .expected_email = "email",
      .expected_authentication_token = base::Base64Encode("auth_token"),
      .mojo_expected = mojom::LoginFinalizeResult::New(),
  });
  return kLoginFinalizeSuccess.get();
}

using BraveAccountServiceLoginFinalizeTest =
    BraveAccountServiceTest<LoginFinalizeTestCase>;

}  // namespace

TEST_P(BraveAccountServiceLoginFinalizeTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceLoginFinalizeTest,
    testing::Values(LoginFinalizeEncryptedLoginTokenEmpty(),
                    LoginFinalizeClientMacEmpty(),
                    LoginFinalizeLoginTokenFailedToDecrypt(),
                    LoginFinalizeBodyMissingOrFailedToParse(),
                    LoginFinalizeErrorCodeIsNull(),
                    LoginFinalizeInterimPasswordStateMismatch(),
                    LoginFinalizeInterimPasswordStateNotFound(),
                    LoginFinalizeInterimPasswordStateHasExpired(),
                    LoginFinalizeIncorrectCredentials(),
                    LoginFinalizeIncorrectEmail(),
                    LoginFinalizeIncorrectPassword(),
                    LoginFinalizeServerError(),
                    LoginFinalizeUnknown(),
                    LoginFinalizeAuthTokenEmpty(),
                    LoginFinalizeEmailEmpty(),
                    LoginFinalizeAuthenticationTokenFailedToEncrypt(),
                    LoginFinalizeSuccess()),
    BraveAccountServiceLoginFinalizeTest::kNameGenerator);

struct GetServiceTokenTestCase {
  using Endpoint = ServiceToken;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::GetServiceTokenResultPtr,
                                      mojom::GetServiceTokenErrorPtr>;

  static void Run(const GetServiceTokenTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojom::Authentication& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    pref_service.SetDict(
        prefs::kBraveAccountServiceTokens,
        std::move(test_case.service_tokens_dict).Run(base::Time::Now()));

    if (test_case.set_authentication_token) {
      pref_service.SetString(prefs::kBraveAccountAuthenticationToken,
                             base::Base64Encode("auth_token"));
    }

    task_environment.FastForwardBy(test_case.time_advance);

    std::string expected_service_token =
        test_case.mojo_expected.has_value()
            ? test_case.mojo_expected.value()->serviceToken
            : "";

    authentication.GetServiceToken(
        mojom::Service::kEmailAliases,
        // |callback| resolves the TestFuture in BraveAccountServiceTest with
        // the result. The Then() callback runs immediately after, before the
        // test verifies the future's value.
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, std::string expected_service_token) {
              if (!expected_service_token.empty()) {
                const auto* email_aliases =
                    pref_service->GetDict(prefs::kBraveAccountServiceTokens)
                        .FindDict("email-aliases");
                EXPECT_TRUE(email_aliases);
                const auto* service_token =
                    email_aliases->FindString(prefs::keys::kServiceToken);
                EXPECT_TRUE(service_token);
                EXPECT_EQ(*service_token,
                          base::Base64Encode(expected_service_token));
              }
            },
            base::Unretained(&pref_service),
            std::move(expected_service_token))));

    // The ServiceToken (/v2/auth/service_token) network request's callback will
    // be processed on the next message pump iteration, so this runs before the
    // request completes.
    if (test_case.clear_authentication_token) {
      pref_service.ClearPref(prefs::kBraveAccountAuthenticationToken);
    }
  }

  std::string test_name;
  // |service_tokens_dict| is a callback instead of a plain base::DictValue
  // so that test cases can use the current mock time (passed as parameter)
  // when constructing the dictionary. This is necessary for cache expiration
  // tests that need to set timestamps relative to when the test runs.
  mutable base::OnceCallback<base::DictValue(base::Time)> service_tokens_dict;
  bool set_authentication_token;
  bool fail_decryption;
  bool clear_authentication_token;
  bool fail_encryption;
  base::TimeDelta time_advance;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const GetServiceTokenTestCase* GetServiceTokenCacheHit() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenCacheHit({
          .test_name = "get_service_token_cache_hit",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken,
                         base::Base64Encode("cached_service_token"))
                    .Set(prefs::keys::kLastFetched,
                         base::TimeToValue(mock_now)));
          }),
          .set_authentication_token = {},    // not used
          .fail_decryption = {},             // not used
          .clear_authentication_token = {},  // not used
          .fail_encryption = {},             // not used
          .time_advance = {},                // not used
          .endpoint_response = {},           // not used
          .mojo_expected =
              mojom::GetServiceTokenResult::New("cached_service_token"),
      });
  return kGetServiceTokenCacheHit.get();
}

const GetServiceTokenTestCase* GetServiceTokenUserNotLoggedIn() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenUserNotLoggedIn({
          .test_name = "get_service_token_user_not_logged_in",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = {},    // not used
          .fail_decryption = {},             // not used
          .clear_authentication_token = {},  // not used
          .fail_encryption = {},             // not used
          .time_advance = {},                // not used
          .endpoint_response = {},           // not used
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              std::nullopt, mojom::GetServiceTokenErrorCode::kUserNotLoggedIn)),
      });
  return kGetServiceTokenUserNotLoggedIn.get();
}

const GetServiceTokenTestCase*
GetServiceTokenAuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenAuthenticationTokenDecryptionFailed({
          .test_name =
              "get_service_token_authentication_token_decryption_failed",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = true,
          .clear_authentication_token = {},  // not used
          .fail_encryption = {},             // not used
          .time_advance = {},                // not used
          .endpoint_response = {},           // not used
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              std::nullopt, mojom::GetServiceTokenErrorCode::
                                kAuthenticationTokenDecryptionFailed)),
      });
  return kGetServiceTokenAuthenticationTokenDecryptionFailed.get();
}

const GetServiceTokenTestCase* GetServiceTokenAuthenticationSessionChanged() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenAuthenticationSessionChanged({
          .test_name = "get_service_token_authentication_session_changed",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = true,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              std::nullopt,
              mojom::GetServiceTokenErrorCode::kAuthenticationSessionChanged)),
      });
  return kGetServiceTokenAuthenticationSessionChanged.get();
}

const GetServiceTokenTestCase* GetServiceTokenNetworkError() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenNetworkError({
          .test_name = "get_service_token_network_error",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::ERR_CONNECTION_REFUSED,
                                 .status_code = std::nullopt,
                                 .body = std::nullopt}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::ERR_CONNECTION_REFUSED, std::nullopt)),
      });
  return kGetServiceTokenNetworkError.get();
}

const GetServiceTokenTestCase* GetServiceTokenBodyMissingOrFailedToParse() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenBodyMissingOrFailedToParse({
          .test_name = "get_service_token_body_missing_or_failed_to_parse",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_INTERNAL_SERVER_ERROR, std::nullopt)),
      });
  return kGetServiceTokenBodyMissingOrFailedToParse.get();
}

const GetServiceTokenTestCase* GetServiceTokenErrorCodeIsNull() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenErrorCodeIsNull({
          .test_name = "get_service_token_error_code_is_null",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_BAD_REQUEST, std::nullopt)),
      });
  return kGetServiceTokenErrorCodeIsNull.get();
}

const GetServiceTokenTestCase* GetServiceTokenEmailDomainNotSupported() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenEmailDomainNotSupported({
          .test_name = "get_service_token_email_domain_not_supported",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(13006);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_BAD_REQUEST,
              mojom::GetServiceTokenErrorCode::kEmailDomainNotSupported)),
      });
  return kGetServiceTokenEmailDomainNotSupported.get();
}

const GetServiceTokenTestCase* GetServiceTokenIncorrectCredentials() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenIncorrectCredentials({
          .test_name = "get_service_token_incorrect_credentials",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_FORBIDDEN,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(14004);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_FORBIDDEN,
              mojom::GetServiceTokenErrorCode::kIncorrectCredentials)),
      });
  return kGetServiceTokenIncorrectCredentials.get();
}

const GetServiceTokenTestCase* GetServiceTokenInvalidTokenAudience() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenInvalidTokenAudience({
          .test_name = "get_service_token_invalid_token_audience",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_FORBIDDEN,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(14007);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_FORBIDDEN,
              mojom::GetServiceTokenErrorCode::kInvalidTokenAudience)),
      });
  return kGetServiceTokenInvalidTokenAudience.get();
}

const GetServiceTokenTestCase* GetServiceTokenBadRequest() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenBadRequest({
          .test_name = "get_service_token_bad_request",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_BAD_REQUEST,
              mojom::GetServiceTokenErrorCode::kMiscServerError)),
      });
  return kGetServiceTokenBadRequest.get();
}

const GetServiceTokenTestCase* GetServiceTokenUnauthorized() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenUnauthorized({
          .test_name = "get_service_token_unauthorized",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_UNAUTHORIZED,
              mojom::GetServiceTokenErrorCode::kMiscServerError)),
      });
  return kGetServiceTokenUnauthorized.get();
}

const GetServiceTokenTestCase* GetServiceTokenServerError() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenInternalServerError({
          .test_name = "get_service_token_internal_server_error",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_INTERNAL_SERVER_ERROR,
              mojom::GetServiceTokenErrorCode::kMiscServerError)),
      });
  return kGetServiceTokenInternalServerError.get();
}

const GetServiceTokenTestCase* GetServiceTokenUnknown() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenUnknown({
          .test_name = "get_service_token_unknown",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              net::HTTP_TOO_EARLY, std::nullopt)),
      });
  return kGetServiceTokenUnknown.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenEmpty() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenEmpty({
          .test_name = "get_service_token_service_token_empty",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token = "";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::GetServiceTokenError::New(net::HTTP_OK, std::nullopt)),
      });
  return kGetServiceTokenServiceTokenEmpty.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenEncryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenEncryptionFailed({
          .test_name = "get_service_token_service_token_encryption_failed",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = true,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(mojom::GetServiceTokenError::New(
              std::nullopt,
              mojom::GetServiceTokenErrorCode::kServiceTokenEncryptionFailed)),
      });
  return kGetServiceTokenServiceTokenEncryptionFailed.get();
}

const GetServiceTokenTestCase* GetServiceTokenSuccess() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenSuccess({
          .test_name = "get_service_token_success",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenSuccess.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceDictMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceDictMissing({
          .test_name = "get_service_token_service_dict_missing",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenServiceDictMissing.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenMissing({
          .test_name = "get_service_token_cache_service_token_missing",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue().Set(prefs::keys::kLastFetched,
                                      base::TimeToValue(mock_now)));
          }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenServiceTokenMissing.get();
}

const GetServiceTokenTestCase* GetServiceTokenLastFetchedMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenLastFetchedMissing({
          .test_name = "get_service_token_cache_last_fetched_missing",
          .service_tokens_dict = base::BindOnce([](base::Time) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue().Set(
                    prefs::keys::kServiceToken,
                    base::Base64Encode("cached_service_token")));
          }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenLastFetchedMissing.get();
}

const GetServiceTokenTestCase* GetServiceTokenLastFetchedInvalid() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenLastFetchedInvalid({
          .test_name = "get_service_token_cache_last_fetched_invalid",
          .service_tokens_dict = base::BindOnce([](base::Time) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken,
                         base::Base64Encode("cached_service_token"))
                    .Set(prefs::keys::kLastFetched, "invalid-time-format"));
          }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenLastFetchedInvalid.get();
}

const GetServiceTokenTestCase* GetServiceTokenCacheExpired() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenCacheExpired({
          .test_name = "get_service_token_cache_expired",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken,
                         base::Base64Encode("cached_service_token"))
                    .Set(prefs::keys::kLastFetched,
                         base::TimeToValue(mock_now)));
          }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = false,
          // Advance time after setting cache to make it expired.
          .time_advance = kServiceTokenMaxAge + base::Minutes(1),
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenCacheExpired.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenDecryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenDecryptionFailed({
          .test_name =
              "get_service_token_cache_service_token_decryption_failed",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken, "!!!invalid-base64!!!")
                    .Set(prefs::keys::kLastFetched,
                         base::TimeToValue(mock_now)));
          }),
          .set_authentication_token = true,
          .fail_decryption = false,
          .clear_authentication_token = false,
          .fail_encryption = false,
          .time_advance = base::TimeDelta(),
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenServiceTokenDecryptionFailed.get();
}

using BraveAccountServiceGetServiceTokenTest =
    BraveAccountServiceTest<GetServiceTokenTestCase>;

}  // namespace

TEST_P(BraveAccountServiceGetServiceTokenTest, GetServiceToken) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceGetServiceTokenTest,
    testing::Values(GetServiceTokenCacheHit(),
                    GetServiceTokenUserNotLoggedIn(),
                    GetServiceTokenAuthenticationTokenDecryptionFailed(),
                    GetServiceTokenAuthenticationSessionChanged(),
                    GetServiceTokenNetworkError(),
                    GetServiceTokenBodyMissingOrFailedToParse(),
                    GetServiceTokenErrorCodeIsNull(),
                    GetServiceTokenEmailDomainNotSupported(),
                    GetServiceTokenIncorrectCredentials(),
                    GetServiceTokenInvalidTokenAudience(),
                    GetServiceTokenBadRequest(),
                    GetServiceTokenUnauthorized(),
                    GetServiceTokenServerError(),
                    GetServiceTokenUnknown(),
                    GetServiceTokenServiceTokenEmpty(),
                    GetServiceTokenServiceTokenEncryptionFailed(),
                    GetServiceTokenSuccess(),
                    GetServiceTokenServiceDictMissing(),
                    GetServiceTokenServiceTokenMissing(),
                    GetServiceTokenLastFetchedMissing(),
                    GetServiceTokenLastFetchedInvalid(),
                    GetServiceTokenCacheExpired(),
                    GetServiceTokenServiceTokenDecryptionFailed()),
    BraveAccountServiceGetServiceTokenTest::kNameGenerator);

}  // namespace brave_account
