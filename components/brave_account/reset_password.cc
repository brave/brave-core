/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/reset_password.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/mojom/reset_password.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "brave/components/brave_account/state_internal.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using internal::MakeClientError;
using internal::MakeRequest;
using internal::MakeServerError;

ResetPassword::ResetPassword(StateBase& state) : state_(state) {}

ResetPassword::~ResetPassword() = default;

void ResetPassword::VerifyInit(
    const std::string& email,
    mojom::Authentication::ResetPasswordVerifyInitCallback callback) {
  CHECK(!email.empty());

  auto request = MakeRequest<endpoints::VerifyInit::Request>();
  request.body.email = email;
  request.body.intent = "reset_password";
  // Server side will determine locale based on the Accept-Language request
  // header (which is included automatically by upstream).
  request.body.locale = "";
  request.body.service = kServiceToString.at(mojom::Service::kAccounts);

  state_->SendStateOwnedRequest<endpoints::VerifyInit>(
      std::move(request),
      base::BindOnce(&ResetPassword::OnVerifyInit, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void ResetPassword::VerifyComplete(
    const std::string& code,
    mojom::Authentication::ResetPasswordVerifyCompleteCallback callback) {
  CHECK(!code.empty());

  auto verification_token =
      state_->GetDecryptedVerificationToken<mojom::ResetPasswordError>(
          mojom::VerificationIntent::NewLoggedOutIntent(
              mojom::LoggedOutVerificationIntent::kResetPassword));
  if (!verification_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(verification_token).error()));
  }

  auto request = MakeRequest<WithHeaders<endpoints::VerifyComplete::Request>>();
  SetBearerToken(request, *verification_token);
  request.body.code = code;

  state_->SendStateOwnedRequest<endpoints::VerifyComplete>(
      std::move(request),
      base::BindOnce(&ResetPassword::OnVerifyComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ResetPassword::PasswordInit(
    const std::string& blinded_message,
    mojom::Authentication::ResetPasswordPasswordInitCallback callback) {
  CHECK(!blinded_message.empty());

  auto verification_token =
      state_->GetDecryptedVerificationToken<mojom::ResetPasswordError>(
          mojom::VerificationIntent::NewLoggedOutIntent(
              mojom::LoggedOutVerificationIntent::kResetPassword));
  if (!verification_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(verification_token).error()));
  }

  auto request = MakeRequest<WithHeaders<endpoints::PasswordInit::Request>>();
  SetBearerToken(request, *verification_token);
  request.body.blinded_message = blinded_message;
  request.body.initiating_service_name =
      kServiceToString.at(mojom::Service::kAccounts);
  request.body.serialize_response = true;

  state_->SendStateOwnedRequest<endpoints::PasswordInit>(
      std::move(request),
      base::BindOnce(&ResetPassword::OnPasswordInit, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void ResetPassword::PasswordFinalize(
    const std::string& serialized_record,
    const std::string& email,
    mojom::Authentication::ResetPasswordPasswordFinalizeCallback callback) {
  CHECK(!serialized_record.empty());
  CHECK(!email.empty());

  auto verification_token =
      state_->GetDecryptedVerificationToken<mojom::ResetPasswordError>(
          mojom::VerificationIntent::NewLoggedOutIntent(
              mojom::LoggedOutVerificationIntent::kResetPassword));
  if (!verification_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(verification_token).error()));
  }

  auto request =
      MakeRequest<WithHeaders<endpoints::PasswordFinalize::Request>>();
  SetBearerToken(request, *verification_token);
  request.body.serialized_record = serialized_record;

  state_->SendStateOwnedRequest<endpoints::PasswordFinalize>(
      std::move(request),
      base::BindOnce(&ResetPassword::OnPasswordFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback), email));
}

void ResetPassword::OnVerifyInit(
    mojom::Authentication::ResetPasswordVerifyInitCallback callback,
    endpoints::VerifyInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResetPasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ResetPasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  std::string encrypted_verification_token;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody            ]> ==>
          // expected<SuccessBody, [ResetPasswordErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ResetPasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                     ], ResetPasswordErrorPtr>
          // ==>
          // expected<[ResetPasswordVerifyInitResultPtr], ResetPasswordErrorPtr>
          .and_then(
              [&](auto success_body)
                  -> base::expected<mojom::ResetPasswordVerifyInitResultPtr,
                                    mojom::ResetPasswordErrorPtr> {
                if (success_body.verification_token.empty()) {
                  return base::unexpected(MakeServerError<
                                          mojom::ResetPasswordError>(
                      status_code,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse));
                }

                if (encrypted_verification_token =
                        state_->Encrypt(success_body.verification_token);
                    encrypted_verification_token.empty()) {
                  return base::unexpected(
                      MakeClientError<mojom::ResetPasswordError>(
                          mojom::ResetPasswordClientErrorCode::
                              kVerificationTokenEncryptionFailed));
                }

                return mojom::ResetPasswordVerifyInitResult::New();
              });

  // See `StateBase`'s class comment on ordering.
  // LoggedOut ==> LoggedOutWithVerification (no state swap).
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!encrypted_verification_token.empty());
    state_->account_state_prefs_->AddVerification(
        encrypted_verification_token,
        mojom::VerificationIntent::NewLoggedOutIntent(
            mojom::LoggedOutVerificationIntent::kResetPassword));
  }
}

void ResetPassword::OnVerifyComplete(
    mojom::Authentication::ResetPasswordVerifyCompleteCallback callback,
    endpoints::VerifyComplete::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResetPasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ResetPasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  std::string email;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody         ]> ==>
          // expected<SuccessBody, [ResetPasswordError]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ResetPasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                         ],
          //           ResetPasswordErrorPtr> ==>
          // expected<[ResetPasswordVerifyCompleteResultPtr],
          //           ResetPasswordErrorPtr>
          .and_then(
              [&](auto success_body)
                  -> base::expected<mojom::ResetPasswordVerifyCompleteResultPtr,
                                    mojom::ResetPasswordErrorPtr> {
                if (success_body.email.empty()) {
                  return base::unexpected(MakeServerError<
                                          mojom::ResetPasswordError>(
                      status_code,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse));
                }

                email = std::move(success_body.email);

                return mojom::ResetPasswordVerifyCompleteResult::New();
              });

  // See `StateBase`'s class comment on ordering.
  // LoggedOutWithVerification ==> LoggedOutWithVerification (no state swap):
  // records the verified email.
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!email.empty());
    state_->account_state_prefs_->SetVerificationVerifiedEmail(email);
  }
}

void ResetPassword::OnPasswordInit(
    mojom::Authentication::ResetPasswordPasswordInitCallback callback,
    endpoints::PasswordInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResetPasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ResetPasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [ResetPasswordErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ResetPasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                       ],
          //           ResetPasswordErrorPtr> ==>
          // expected<[ResetPasswordPasswordInitResultPtr],
          //           ResetPasswordErrorPtr>
          .and_then(
              [&](auto success_body)
                  -> base::expected<mojom::ResetPasswordPasswordInitResultPtr,
                                    mojom::ResetPasswordErrorPtr> {
                if (success_body.serialized_response.empty()) {
                  return base::unexpected(MakeServerError<
                                          mojom::ResetPasswordError>(
                      status_code,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse));
                }

                return mojom::ResetPasswordPasswordInitResult::New(
                    std::move(success_body.serialized_response));
              });

  std::move(callback).Run(std::move(result));
}

void ResetPassword::OnPasswordFinalize(
    mojom::Authentication::ResetPasswordPasswordFinalizeCallback callback,
    const std::string& email,
    endpoints::PasswordFinalize::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResetPasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ResetPasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  std::string encrypted_authentication_token;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [ResetPasswordErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ResetPasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                           ],
          //           ResetPasswordErrorPtr> ==>
          // expected<[ResetPasswordPasswordFinalizeResultPtr],
          //           ResetPasswordErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<
                            mojom::ResetPasswordPasswordFinalizeResultPtr,
                            mojom::ResetPasswordErrorPtr> {
            if (!success_body.auth_token.is_string() ||
                success_body.auth_token.GetString().empty()) {
              return base::unexpected(
                  MakeServerError<mojom::ResetPasswordError>(
                      status_code,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse));
            }

            if (encrypted_authentication_token =
                    state_->Encrypt(success_body.auth_token.GetString());
                encrypted_authentication_token.empty()) {
              return base::unexpected(
                  MakeClientError<mojom::ResetPasswordError>(
                      mojom::ResetPasswordClientErrorCode::
                          kAuthenticationTokenEncryptionFailed));
            }

            return mojom::ResetPasswordPasswordFinalizeResult::New();
          });

  // See `StateBase`'s class comment on ordering.
  // LoggedOutWithVerification ==> LoggedIn (state swap).
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!encrypted_authentication_token.empty());
    state_->account_state_prefs_->SetLoggedIn(email,
                                              encrypted_authentication_token);
  }
}

}  // namespace brave_account
