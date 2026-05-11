/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/logged_out_state.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/verify_delete.h"
#include "brave/components/brave_account/state_internal.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::LoginFinalize;
using endpoints::LoginInit;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyComplete;
using endpoints::VerifyDelete;
using endpoints::VerifyResend;
using internal::MakeClientError;
using internal::MakeRequest;
using internal::MakeServerError;

LoggedOutState::LoggedOutState(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor,
    AddObserverCallback add_observer)
    : StateBase(account_state_prefs,
                std::move(url_loader_factory),
                encryptor,
                std::move(add_observer)) {}

LoggedOutState::~LoggedOutState() = default;

void LoggedOutState::RegisterInitialize(mojom::Service initiating_service,
                                        const std::string& email,
                                        const std::string& blinded_message,
                                        RegisterInitializeCallback callback) {
  CHECK(!email.empty());
  CHECK(!blinded_message.empty());

  auto request = MakeRequest<PasswordInit::Request>();
  request.body.blinded_message = blinded_message;
  request.body.initiating_service_name =
      kServiceToString.at(initiating_service);
  request.body.new_account_email = email;
  request.body.serialize_response = true;

  SendStateOwnedRequest<PasswordInit>(
      std::move(request),
      base::BindOnce(&LoggedOutState::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void LoggedOutState::RegisterFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    RegisterFinalizeCallback callback) {
  CHECK(!encrypted_verification_token.empty());
  CHECK(!serialized_record.empty());

  const std::string verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(base::unexpected(MakeClientError<
                                                    mojom::RegisterError>(
        mojom::RegisterClientErrorCode::kVerificationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<PasswordFinalize::Request>>();
  SetBearerToken(request, verification_token);
  request.body.serialized_record = serialized_record;

  SendStateOwnedRequest<PasswordFinalize>(
      std::move(request),
      base::BindOnce(&LoggedOutState::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     encrypted_verification_token));
}

void LoggedOutState::RegisterVerify(const std::string& code,
                                    RegisterVerifyCallback callback) {
  CHECK(!code.empty());

  const auto encrypted_verification_token =
      account_state_prefs_->GetVerificationToken(
          mojom::LoggedOutVerificationIntent::kRegistration);
  CHECK(!encrypted_verification_token.empty());
  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(base::unexpected(MakeClientError<
                                                    mojom::RegisterError>(
        mojom::RegisterClientErrorCode::kVerificationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<VerifyComplete::Request>>();
  SetBearerToken(request, verification_token);
  request.body.code = code;

  SendStateOwnedRequest<VerifyComplete>(
      std::move(request),
      base::BindOnce(&LoggedOutState::OnRegisterVerify,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void LoggedOutState::ResendConfirmationEmail(
    ResendConfirmationEmailCallback callback) {
  const auto encrypted_verification_token =
      account_state_prefs_->GetVerificationToken(
          mojom::LoggedOutVerificationIntent::kRegistration);
  CHECK(!encrypted_verification_token.empty());
  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::ResendConfirmationEmailError>(
            mojom::ResendConfirmationEmailClientErrorCode::
                kVerificationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<VerifyResend::Request>>();
  SetBearerToken(request, verification_token);
  // Server side will determine locale based on the Accept-Language request
  // header (which is included automatically by upstream).
  request.body.locale = "";
  request.timeout_duration = kVerifyResendTimeout;

  SendStateOwnedRequest<VerifyResend>(
      std::move(request),
      base::BindOnce(&LoggedOutState::OnResendConfirmationEmail,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void LoggedOutState::CancelRegistration() {
  // Best-effort notification to the server, since server side will clean up
  // verification tokens automatically (currently after 30 minutes).
  // Not adopted into the state's in-flight bag:
  // best-effort with no callback that touches state.
  const auto encrypted_verification_token =
      account_state_prefs_->GetVerificationToken(
          mojom::LoggedOutVerificationIntent::kRegistration);
  CHECK(!encrypted_verification_token.empty());
  if (const auto verification_token = Decrypt(encrypted_verification_token);
      !verification_token.empty()) {
    auto request = MakeRequest<WithHeaders<VerifyDelete::Request>>();
    SetBearerToken(request, verification_token);

    SendUnownedRequest<VerifyDelete>(std::move(request));
  }

  // LoggedOutWithVerification ==> LoggedOut (no state swap)
  account_state_prefs_->SetLoggedOut();
}

void LoggedOutState::LoginInitialize(mojom::Service initiating_service,
                                     const std::string& email,
                                     const std::string& serialized_ke1,
                                     LoginInitializeCallback callback) {
  CHECK(!email.empty());
  CHECK(!serialized_ke1.empty());

  auto request = MakeRequest<LoginInit::Request>();
  request.body.email = email;
  request.body.initiating_service_name =
      kServiceToString.at(initiating_service);
  request.body.serialized_ke1 = serialized_ke1;

  SendStateOwnedRequest<LoginInit>(
      std::move(request),
      base::BindOnce(&LoggedOutState::OnLoginInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void LoggedOutState::LoginFinalize(const std::string& encrypted_login_token,
                                   const std::string& client_mac,
                                   LoginFinalizeCallback callback) {
  CHECK(!encrypted_login_token.empty());
  CHECK(!client_mac.empty());

  const std::string login_token = Decrypt(encrypted_login_token);
  if (login_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::LoginError>(
            mojom::LoginClientErrorCode::kLoginTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<LoginFinalize::Request>>();
  SetBearerToken(request, login_token);
  request.body.client_mac = client_mac;

  SendStateOwnedRequest<endpoints::LoginFinalize>(
      std::move(request),
      base::BindOnce(&LoggedOutState::OnLoginFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void LoggedOutState::OnRegisterInitialize(RegisterInitializeCallback callback,
                                          PasswordInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::RegisterError>(
            response.status_code.value_or(response.net_error),
            mojom::RegisterServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::RegisterError>(status_code,
                                                         std::move(error_body));
          })
          // expected<[SuccessBody                ], RegisterErrorPtr> ==>
          // expected<[RegisterInitializeResultPtr], RegisterErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterInitializeResultPtr,
                                          mojom::RegisterErrorPtr> {
            if (success_body.verification_token.empty() ||
                success_body.serialized_response.empty()) {
              return base::unexpected(MakeServerError<mojom::RegisterError>(
                  status_code,
                  mojom::RegisterServerErrorCode::kInvalidResponse));
            }

            std::string encrypted_verification_token =
                Encrypt(success_body.verification_token);
            if (encrypted_verification_token.empty()) {
              return base::unexpected(MakeClientError<mojom::RegisterError>(
                  mojom::RegisterClientErrorCode::
                      kVerificationTokenEncryptionFailed));
            }

            return mojom::RegisterInitializeResult::New(
                std::move(encrypted_verification_token),
                std::move(success_body.serialized_response));
          });

  std::move(callback).Run(std::move(result));
}

void LoggedOutState::OnRegisterFinalize(
    RegisterFinalizeCallback callback,
    const std::string& encrypted_verification_token,
    PasswordFinalize::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::RegisterError>(
            response.status_code.value_or(response.net_error),
            mojom::RegisterServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::RegisterError>(status_code,
                                                         std::move(error_body));
          })
          // expected<[SuccessBody              ], RegisterErrorPtr> ==>
          // expected<[RegisterFinalizeResultPtr], RegisterErrorPtr>
          .and_then([](auto success_body)
                        -> base::expected<mojom::RegisterFinalizeResultPtr,
                                          mojom::RegisterErrorPtr> {
            return mojom::RegisterFinalizeResult::New();
          });

  // See `StateBase`'s class comment on ordering.
  // LoggedOut ==> LoggedOutWithVerification (no state swap).
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    account_state_prefs_->SetLoggedOutWithVerification(
        encrypted_verification_token,
        mojom::LoggedOutVerificationIntent::kRegistration);
  }
}

void LoggedOutState::OnRegisterVerify(RegisterVerifyCallback callback,
                                      VerifyComplete::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::RegisterError>(
            response.status_code.value_or(response.net_error),
            mojom::RegisterServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  std::string email;
  std::string encrypted_authentication_token;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::RegisterError>(status_code,
                                                         std::move(error_body));
          })
          // expected<[SuccessBody            ], RegisterErrorPtr> ==>
          // expected<[RegisterVerifyResultPtr], RegisterErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterVerifyResultPtr,
                                          mojom::RegisterErrorPtr> {
            if (success_body.auth_token.empty() || success_body.email.empty()) {
              return base::unexpected(MakeServerError<mojom::RegisterError>(
                  status_code,
                  mojom::RegisterServerErrorCode::kInvalidResponse));
            }

            if (encrypted_authentication_token =
                    Encrypt(success_body.auth_token);
                encrypted_authentication_token.empty()) {
              return base::unexpected(MakeClientError<mojom::RegisterError>(
                  mojom::RegisterClientErrorCode::
                      kAuthenticationTokenEncryptionFailed));
            }

            email = std::move(success_body.email);

            return mojom::RegisterVerifyResult::New();
          });

  // See `StateBase`'s class comment on ordering.
  // LoggedOutWithVerification ==> LoggedIn (state swap).
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!email.empty());
    CHECK(!encrypted_authentication_token.empty());
    account_state_prefs_->SetLoggedIn(email, encrypted_authentication_token);
  }
}

void LoggedOutState::OnResendConfirmationEmail(
    ResendConfirmationEmailCallback callback,
    VerifyResend::Response response) {
  if (response.status_code == net::HTTP_NO_CONTENT) {
    return std::move(callback).Run(mojom::ResendConfirmationEmailResult::New());
  }

  if (!response.body || response.body->has_value()) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResendConfirmationEmailError>(
            response.status_code.value_or(response.net_error),
            mojom::ResendConfirmationEmailServerErrorCode::kInvalidResponse)));
  }

  std::move(callback).Run(
      base::unexpected(MakeServerError<mojom::ResendConfirmationEmailError>(
          CHECK_DEREF(response.status_code),
          std::move(response.body->error()))));
}

void LoggedOutState::OnLoginInitialize(LoginInitializeCallback callback,
                                       LoginInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::LoginError>(
            response.status_code.value_or(response.net_error),
            mojom::LoginServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [LoginErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::LoginError>(status_code,
                                                      std::move(error_body));
          })
          // expected<[SuccessBody             ], LoginErrorPtr> ==>
          // expected<[LoginInitializeResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginInitializeResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.login_token.empty() ||
                success_body.serialized_ke2.empty()) {
              return base::unexpected(MakeServerError<mojom::LoginError>(
                  status_code, mojom::LoginServerErrorCode::kInvalidResponse));
            }

            std::string encrypted_login_token =
                Encrypt(success_body.login_token);
            if (encrypted_login_token.empty()) {
              return base::unexpected(MakeClientError<mojom::LoginError>(
                  mojom::LoginClientErrorCode::kLoginTokenEncryptionFailed));
            }

            return mojom::LoginInitializeResult::New(
                std::move(encrypted_login_token),
                std::move(success_body.serialized_ke2));
          });

  std::move(callback).Run(std::move(result));
}

void LoggedOutState::OnLoginFinalize(LoginFinalizeCallback callback,
                                     LoginFinalize::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::LoginError>(
            response.status_code.value_or(response.net_error),
            mojom::LoginServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  std::string email;
  std::string encrypted_authentication_token;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [LoginErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::LoginError>(status_code,
                                                      std::move(error_body));
          })
          // expected<[SuccessBody           ], LoginErrorPtr> ==>
          // expected<[LoginFinalizeResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginFinalizeResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.auth_token.empty() || success_body.email.empty()) {
              return base::unexpected(MakeServerError<mojom::LoginError>(
                  status_code, mojom::LoginServerErrorCode::kInvalidResponse));
            }

            if (encrypted_authentication_token =
                    Encrypt(success_body.auth_token);
                encrypted_authentication_token.empty()) {
              return base::unexpected(MakeClientError<mojom::LoginError>(
                  mojom::LoginClientErrorCode::
                      kAuthenticationTokenEncryptionFailed));
            }

            email = std::move(success_body.email);

            return mojom::LoginFinalizeResult::New();
          });

  // See `StateBase`'s class comment on ordering.
  // LoggedOut ==> LoggedIn (state swap).
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!email.empty());
    CHECK(!encrypted_authentication_token.empty());
    account_state_prefs_->SetLoggedIn(email, encrypted_authentication_token);
  }
}

}  // namespace brave_account
