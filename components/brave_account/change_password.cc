/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/change_password.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/state_base.h"
#include "brave/components/brave_account/state_internal.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using internal::MakeClientError;
using internal::MakeRequest;
using internal::MakeServerError;

ChangePassword::ChangePassword(StateBase& state) : state_(state) {}

ChangePassword::~ChangePassword() = default;

void ChangePassword::VerifyInit(
    const std::string& email,
    mojom::Authentication::ChangePasswordVerifyInitCallback callback) {
  CHECK(!email.empty());

  auto authentication_token =
      state_->GetDecryptedAuthenticationToken<mojom::ChangePasswordError>();
  if (!authentication_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(authentication_token).error()));
  }

  auto request = MakeRequest<WithHeaders<endpoints::VerifyInit::Request>>();
  SetBearerToken(request, *authentication_token);
  request.body.email = email;
  request.body.intent = "change_password";
  // Server side will determine locale based on the Accept-Language request
  // header (which is included automatically by upstream).
  request.body.locale = "";
  request.body.service = kServiceToString.at(mojom::Service::kAccounts);

  state_->SendStateOwnedRequest<endpoints::VerifyInit>(
      std::move(request),
      base::BindOnce(&ChangePassword::OnVerifyInit, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void ChangePassword::VerifyComplete(
    const std::string& code,
    mojom::Authentication::ChangePasswordVerifyCompleteCallback callback) {
  CHECK(!code.empty());

  auto verification_token =
      state_->GetDecryptedVerificationToken<mojom::ChangePasswordError>(
          mojom::VerificationIntent::NewLoggedInIntent(
              mojom::LoggedInVerificationIntent::kChangePassword));
  if (!verification_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(verification_token).error()));
  }

  auto request = MakeRequest<WithHeaders<endpoints::VerifyComplete::Request>>();
  SetBearerToken(request, *verification_token);
  request.body.code = code;

  state_->SendStateOwnedRequest<endpoints::VerifyComplete>(
      std::move(request),
      base::BindOnce(&ChangePassword::OnVerifyComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ChangePassword::PasswordInit(
    const std::string& blinded_message,
    mojom::Authentication::ChangePasswordPasswordInitCallback callback) {
  CHECK(!blinded_message.empty());

  auto verification_token =
      state_->GetDecryptedVerificationToken<mojom::ChangePasswordError>(
          mojom::VerificationIntent::NewLoggedInIntent(
              mojom::LoggedInVerificationIntent::kChangePassword));
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
      base::BindOnce(&ChangePassword::OnPasswordInit,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ChangePassword::PasswordFinalize(
    const std::string& serialized_record,
    mojom::Authentication::ChangePasswordPasswordFinalizeCallback callback) {
  CHECK(!serialized_record.empty());

  auto verification_token =
      state_->GetDecryptedVerificationToken<mojom::ChangePasswordError>(
          mojom::VerificationIntent::NewLoggedInIntent(
              mojom::LoggedInVerificationIntent::kChangePassword));
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
      base::BindOnce(&ChangePassword::OnPasswordFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ChangePassword::OnVerifyInit(
    mojom::Authentication::ChangePasswordVerifyInitCallback callback,
    endpoints::VerifyInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ChangePasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ChangePasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  std::string encrypted_verification_token;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody             ]> ==>
          // expected<SuccessBody, [ChangePasswordErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ChangePasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                      ],
          //           ChangePasswordErrorPtr> ==>
          // expected<[ChangePasswordVerifyInitResultPtr],
          //           ChangePasswordErrorPtr>
          .and_then(
              [&](auto success_body)
                  -> base::expected<mojom::ChangePasswordVerifyInitResultPtr,
                                    mojom::ChangePasswordErrorPtr> {
                if (success_body.verification_token.empty()) {
                  return base::unexpected(MakeServerError<
                                          mojom::ChangePasswordError>(
                      status_code,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse));
                }

                if (encrypted_verification_token =
                        state_->Encrypt(success_body.verification_token);
                    encrypted_verification_token.empty()) {
                  return base::unexpected(
                      MakeClientError<mojom::ChangePasswordError>(
                          mojom::ChangePasswordClientErrorCode::
                              kVerificationTokenEncryptionFailed));
                }

                return mojom::ChangePasswordVerifyInitResult::New();
              });

  // See `StateBase`'s class comment on ordering.
  // LoggedIn ==> LoggedInWithVerification (no state swap): attaches a
  // verification slot to the existing logged-in state.
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!encrypted_verification_token.empty());
    state_->account_state_prefs_->AddVerification(
        encrypted_verification_token,
        mojom::VerificationIntent::NewLoggedInIntent(
            mojom::LoggedInVerificationIntent::kChangePassword));
  }
}

void ChangePassword::OnVerifyComplete(
    mojom::Authentication::ChangePasswordVerifyCompleteCallback callback,
    endpoints::VerifyComplete::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ChangePasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ChangePasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  std::string email;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody             ]> ==>
          // expected<SuccessBody, [ChangePasswordError   ]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ChangePasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                          ],
          //           ChangePasswordErrorPtr> ==>
          // expected<[ChangePasswordVerifyCompleteResultPtr],
          //           ChangePasswordErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<
                            mojom::ChangePasswordVerifyCompleteResultPtr,
                            mojom::ChangePasswordErrorPtr> {
            if (success_body.email.empty()) {
              return base::unexpected(
                  MakeServerError<mojom::ChangePasswordError>(
                      status_code,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse));
            }

            email = std::move(success_body.email);

            return mojom::ChangePasswordVerifyCompleteResult::New();
          });

  // See `StateBase`'s class comment on ordering.
  // LoggedInWithVerification ==> LoggedInWithVerification (no state swap):
  // records the verified email.
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!email.empty());
    state_->account_state_prefs_->SetVerificationVerifiedEmail(email);
  }
}

void ChangePassword::OnPasswordInit(
    mojom::Authentication::ChangePasswordPasswordInitCallback callback,
    endpoints::PasswordInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ChangePasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ChangePasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody             ]> ==>
          // expected<SuccessBody, [ChangePasswordErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ChangePasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                        ],
          //           ChangePasswordErrorPtr> ==>
          // expected<[ChangePasswordPasswordInitResultPtr],
          //           ChangePasswordErrorPtr>
          .and_then(
              [&](auto success_body)
                  -> base::expected<mojom::ChangePasswordPasswordInitResultPtr,
                                    mojom::ChangePasswordErrorPtr> {
                if (success_body.serialized_response.empty()) {
                  return base::unexpected(MakeServerError<
                                          mojom::ChangePasswordError>(
                      status_code,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse));
                }

                return mojom::ChangePasswordPasswordInitResult::New(
                    std::move(success_body.serialized_response));
              });

  std::move(callback).Run(std::move(result));
}

void ChangePassword::OnPasswordFinalize(
    mojom::Authentication::ChangePasswordPasswordFinalizeCallback callback,
    endpoints::PasswordFinalize::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ChangePasswordError>(
            response.status_code.value_or(response.net_error),
            mojom::ChangePasswordServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody             ]> ==>
          // expected<SuccessBody, [ChangePasswordErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::ChangePasswordError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                            ],
          //           ChangePasswordErrorPtr> ==>
          // expected<[ChangePasswordPasswordFinalizeResultPtr],
          //           ChangePasswordErrorPtr>
          .and_then([](auto success_body)
                        -> base::expected<
                            mojom::ChangePasswordPasswordFinalizeResultPtr,
                            mojom::ChangePasswordErrorPtr> {
            return mojom::ChangePasswordPasswordFinalizeResult::New();
          });

  // See `StateBase`'s class comment on ordering.
  // LoggedInWithVerification ==> LoggedIn (no state swap): drops the
  // verification slot.
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    state_->account_state_prefs_->ClearVerification();
  }
}

}  // namespace brave_account
