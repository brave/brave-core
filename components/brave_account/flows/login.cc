/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/login.h"

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

Login::Login(StateBase& state) : state_(state) {}

Login::~Login() = default;

void Login::Step1(mojom::Service initiating_service,
                  const std::string& email,
                  const std::string& serialized_ke1,
                  mojom::Authentication::LoginStep1Callback callback) {
  CHECK(!email.empty());
  CHECK(!serialized_ke1.empty());

  auto request = MakeRequest<endpoints::LoginInit::Request>();
  request.body.email = email;
  request.body.initiating_service_name =
      kServiceToString.at(initiating_service);
  request.body.serialized_ke1 = serialized_ke1;

  state_->SendStateOwnedRequest<endpoints::LoginInit>(
      std::move(request),
      base::BindOnce(&Login::OnStep1, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void Login::Step2(const std::string& encrypted_login_token,
                  const std::string& client_mac,
                  mojom::Authentication::LoginStep2Callback callback) {
  CHECK(!encrypted_login_token.empty());
  CHECK(!client_mac.empty());

  const std::string login_token = state_->Decrypt(encrypted_login_token);
  if (login_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::LoginError>(
            mojom::LoginClientErrorCode::kLoginTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<endpoints::LoginFinalize::Request>>();
  SetBearerToken(request, login_token);
  request.body.client_mac = client_mac;

  state_->SendStateOwnedRequest<endpoints::LoginFinalize>(
      std::move(request),
      base::BindOnce(&Login::OnStep2, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void Login::OnStep1(mojom::Authentication::LoginStep1Callback callback,
                    endpoints::LoginInit::Response response) {
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
          // expected<[SuccessBody        ], LoginErrorPtr> ==>
          // expected<[LoginStep1ResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginStep1ResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.login_token.empty() ||
                success_body.serialized_ke2.empty()) {
              return base::unexpected(MakeServerError<mojom::LoginError>(
                  status_code, mojom::LoginServerErrorCode::kInvalidResponse));
            }

            std::string encrypted_login_token =
                state_->Encrypt(success_body.login_token);
            if (encrypted_login_token.empty()) {
              return base::unexpected(MakeClientError<mojom::LoginError>(
                  mojom::LoginClientErrorCode::kLoginTokenEncryptionFailed));
            }

            return mojom::LoginStep1Result::New(
                std::move(encrypted_login_token),
                std::move(success_body.serialized_ke2));
          });

  std::move(callback).Run(std::move(result));
}

void Login::OnStep2(mojom::Authentication::LoginStep2Callback callback,
                    endpoints::LoginFinalize::Response response) {
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
          // expected<[SuccessBody        ], LoginErrorPtr> ==>
          // expected<[LoginStep2ResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginStep2ResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.auth_token.empty() || success_body.email.empty()) {
              return base::unexpected(MakeServerError<mojom::LoginError>(
                  status_code, mojom::LoginServerErrorCode::kInvalidResponse));
            }

            if (encrypted_authentication_token =
                    state_->Encrypt(success_body.auth_token);
                encrypted_authentication_token.empty()) {
              return base::unexpected(MakeClientError<mojom::LoginError>(
                  mojom::LoginClientErrorCode::
                      kAuthenticationTokenEncryptionFailed));
            }

            email = std::move(success_body.email);

            return mojom::LoginStep2Result::New();
          });

  // See `StateBase`'s class comment on ordering.
  // LoggedOut ==> LoggedIn (state swap).
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
