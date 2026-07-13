/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/register.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/mojom/register.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "brave/components/brave_account/state_internal.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using internal::MakeClientError;
using internal::MakeRequest;
using internal::MakeServerError;

Register::Register(StateBase& state) : state_(state) {}

Register::~Register() = default;

void Register::PasswordInit(
    mojom::Service initiating_service,
    const std::string& email,
    const std::string& blinded_message,
    mojom::Authentication::RegisterPasswordInitCallback callback) {
  CHECK(!email.empty());
  CHECK(!blinded_message.empty());

  auto request = MakeRequest<endpoints::PasswordInit::Request>();
  request.body.blinded_message = blinded_message;
  request.body.initiating_service_name =
      kServiceToString.at(initiating_service);
  request.body.new_account_email = email;
  request.body.serialize_response = true;

  state_->SendStateOwnedRequest<endpoints::PasswordInit>(
      std::move(request),
      base::BindOnce(&Register::OnPasswordInit, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void Register::PasswordFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    mojom::Authentication::RegisterPasswordFinalizeCallback callback) {
  CHECK(!encrypted_verification_token.empty());
  CHECK(!serialized_record.empty());

  const std::string verification_token =
      state_->Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(base::unexpected(MakeClientError<
                                                    mojom::RegisterError>(
        mojom::RegisterClientErrorCode::kVerificationTokenDecryptionFailed)));
  }

  auto request =
      MakeRequest<WithHeaders<endpoints::PasswordFinalize::Request>>();
  SetBearerToken(request, verification_token);
  request.body.serialized_record = serialized_record;

  state_->SendStateOwnedRequest<endpoints::PasswordFinalize>(
      std::move(request),
      base::BindOnce(&Register::OnPasswordFinalize, weak_factory_.GetWeakPtr(),
                     std::move(callback), encrypted_verification_token));
}

void Register::VerifyComplete(
    const std::string& code,
    mojom::Authentication::RegisterVerifyCompleteCallback callback) {
  CHECK(!code.empty());

  auto verification_token =
      state_->GetDecryptedVerificationToken<mojom::RegisterError>(
          mojom::VerificationIntent::NewLoggedOutIntent(
              mojom::LoggedOutVerificationIntent::kRegistration));
  if (!verification_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(verification_token).error()));
  }

  auto request = MakeRequest<WithHeaders<endpoints::VerifyComplete::Request>>();
  SetBearerToken(request, *verification_token);
  request.body.code = code;

  state_->SendStateOwnedRequest<endpoints::VerifyComplete>(
      std::move(request),
      base::BindOnce(&Register::OnVerifyComplete, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void Register::OnPasswordInit(
    mojom::Authentication::RegisterPasswordInitCallback callback,
    endpoints::PasswordInit::Response response) {
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
          // expected<[SuccessBody                  ], RegisterErrorPtr> ==>
          // expected<[RegisterPasswordInitResultPtr], RegisterErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterPasswordInitResultPtr,
                                          mojom::RegisterErrorPtr> {
            if (!success_body.verification_token ||
                success_body.verification_token->empty() ||
                success_body.serialized_response.empty()) {
              return base::unexpected(MakeServerError<mojom::RegisterError>(
                  status_code,
                  mojom::RegisterServerErrorCode::kInvalidResponse));
            }

            std::string encrypted_verification_token =
                state_->Encrypt(*success_body.verification_token);
            if (encrypted_verification_token.empty()) {
              return base::unexpected(MakeClientError<mojom::RegisterError>(
                  mojom::RegisterClientErrorCode::
                      kVerificationTokenEncryptionFailed));
            }

            return mojom::RegisterPasswordInitResult::New(
                std::move(encrypted_verification_token),
                std::move(success_body.serialized_response));
          });

  std::move(callback).Run(std::move(result));
}

void Register::OnPasswordFinalize(
    mojom::Authentication::RegisterPasswordFinalizeCallback callback,
    const std::string& encrypted_verification_token,
    endpoints::PasswordFinalize::Response response) {
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
          // expected<[SuccessBody                      ], RegisterErrorPtr> ==>
          // expected<[RegisterPasswordFinalizeResultPtr], RegisterErrorPtr>
          .and_then(
              [](auto success_body)
                  -> base::expected<mojom::RegisterPasswordFinalizeResultPtr,
                                    mojom::RegisterErrorPtr> {
                return mojom::RegisterPasswordFinalizeResult::New();
              });

  // See `StateBase`'s class comment on ordering.
  // LoggedOut ==> LoggedOutWithVerification (no state swap).
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    state_->account_state_prefs_->AddVerification(
        encrypted_verification_token,
        mojom::VerificationIntent::NewLoggedOutIntent(
            mojom::LoggedOutVerificationIntent::kRegistration));
  }
}

void Register::OnVerifyComplete(
    mojom::Authentication::RegisterVerifyCompleteCallback callback,
    endpoints::VerifyComplete::Response response) {
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
          // expected<[SuccessBody                    ], RegisterErrorPtr> ==>
          // expected<[RegisterVerifyCompleteResultPtr], RegisterErrorPtr>
          .and_then(
              [&](auto success_body)
                  -> base::expected<mojom::RegisterVerifyCompleteResultPtr,
                                    mojom::RegisterErrorPtr> {
                if (!success_body.auth_token.is_string() ||
                    success_body.auth_token.GetString().empty() ||
                    success_body.email.empty()) {
                  return base::unexpected(MakeServerError<mojom::RegisterError>(
                      status_code,
                      mojom::RegisterServerErrorCode::kInvalidResponse));
                }

                if (encrypted_authentication_token =
                        state_->Encrypt(success_body.auth_token.GetString());
                    encrypted_authentication_token.empty()) {
                  return base::unexpected(MakeClientError<mojom::RegisterError>(
                      mojom::RegisterClientErrorCode::
                          kAuthenticationTokenEncryptionFailed));
                }

                email = std::move(success_body.email);

                return mojom::RegisterVerifyCompleteResult::New();
              });

  // See `StateBase`'s class comment on ordering.
  // LoggedOutWithVerification ==> LoggedIn (state swap).
  const bool success = result.has_value();
  std::move(callback).Run(std::move(result));

  if (success) {
    CHECK(!email.empty());
    CHECK(!encrypted_authentication_token.empty());
    state_->account_state_prefs_->SetLoggedIn(email,
                                              encrypted_authentication_token);
  }
}

}  // namespace brave_account
