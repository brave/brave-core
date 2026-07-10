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
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/state_internal.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::LoginFinalize;
using endpoints::LoginInit;
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

void LoggedOutState::RegisterPasswordInit(
    mojom::Service initiating_service,
    const std::string& email,
    const std::string& blinded_message,
    RegisterPasswordInitCallback callback) {
  register_.PasswordInit(initiating_service, email, blinded_message,
                         std::move(callback));
}

void LoggedOutState::RegisterPasswordFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    RegisterPasswordFinalizeCallback callback) {
  register_.PasswordFinalize(encrypted_verification_token, serialized_record,
                             std::move(callback));
}

void LoggedOutState::RegisterVerifyComplete(
    const std::string& code,
    RegisterVerifyCompleteCallback callback) {
  register_.VerifyComplete(code, std::move(callback));
}

void LoggedOutState::ResetPasswordVerifyInit(
    const std::string& email,
    ResetPasswordVerifyInitCallback callback) {
  reset_password_.VerifyInit(email, std::move(callback));
}

void LoggedOutState::ResetPasswordVerifyComplete(
    const std::string& code,
    ResetPasswordVerifyCompleteCallback callback) {
  reset_password_.VerifyComplete(code, std::move(callback));
}

void LoggedOutState::ResetPasswordPasswordInit(
    const std::string& blinded_message,
    ResetPasswordPasswordInitCallback callback) {
  reset_password_.PasswordInit(blinded_message, std::move(callback));
}

void LoggedOutState::ResetPasswordPasswordFinalize(
    const std::string& serialized_record,
    const std::string& email,
    ResetPasswordPasswordFinalizeCallback callback) {
  reset_password_.PasswordFinalize(serialized_record, email,
                                   std::move(callback));
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
