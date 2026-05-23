/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/logged_in_state.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/auth_logout.h"
#include "brave/components/brave_account/state_internal.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::RequestHandle;
using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::AuthLogout;
using endpoints::AuthValidate;
using endpoints::ServiceToken;
using internal::MakeClientError;
using internal::MakeRequest;
using internal::MakeServerError;

LoggedInState::LoggedInState(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor,
    AddObserverCallback add_observer)
    : StateBase(account_state_prefs,
                std::move(url_loader_factory),
                encryptor,
                std::move(add_observer)) {
  ScheduleAuthValidate();
}

LoggedInState::~LoggedInState() = default;

void LoggedInState::LogOut() {
  // Best-effort notification to the server, since server side will clean up
  // authentication tokens automatically (currently in 6 months of inactivity).
  // Not adopted into the state's in-flight bag:
  // best-effort with no callback that touches state.
  const auto encrypted_authentication_token =
      account_state_prefs_->GetAuthenticationToken();
  CHECK(!encrypted_authentication_token.empty());
  if (const auto authentication_token = Decrypt(encrypted_authentication_token);
      !authentication_token.empty()) {
    auto request = MakeRequest<WithHeaders<AuthLogout::Request>>();
    SetBearerToken(request, authentication_token);

    SendUnownedRequest<AuthLogout>(std::move(request));
  }

  // See `StateBase`'s class comment on ordering.
  // LoggedIn ==> LoggedOut (state swap).
  account_state_prefs_->SetLoggedOut();
}

void LoggedInState::GetServiceToken(mojom::Service service,
                                    GetServiceTokenCallback callback) {
  CHECK(service != mojom::Service::kAccounts);
  std::string service_name(kServiceToString.at(service));
  if (auto service_token =
          Decrypt(account_state_prefs_->GetCachedServiceToken(service_name));
      !service_token.empty()) {
    return std::move(callback).Run(
        mojom::GetServiceTokenResult::New(std::move(service_token)));
  }

  const auto encrypted_authentication_token =
      account_state_prefs_->GetAuthenticationToken();
  CHECK(!encrypted_authentication_token.empty());
  const auto authentication_token = Decrypt(encrypted_authentication_token);
  if (authentication_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::GetServiceTokenError>(
            mojom::GetServiceTokenClientErrorCode::
                kAuthenticationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<ServiceToken::Request>>();
  SetBearerToken(request, authentication_token);
  request.body.service = service_name;

  SendStateOwnedRequest<ServiceToken>(
      std::move(request),
      base::BindOnce(&LoggedInState::OnGetServiceToken,
                     weak_factory_.GetWeakPtr(), std::move(service_name),
                     std::move(callback)));
}

void LoggedInState::OnGetServiceToken(const std::string& service_name,
                                      GetServiceTokenCallback callback,
                                      ServiceToken::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::GetServiceTokenError>(
            response.status_code.value_or(response.net_error),
            mojom::GetServiceTokenServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody              ]> ==>
          // expected<SuccessBody, [GetServiceTokenErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::GetServiceTokenError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody             ], GetServiceTokenErrorPtr> ==>
          // expected<[GetServiceTokenResultPtr], GetServiceTokenErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::GetServiceTokenResultPtr,
                                          mojom::GetServiceTokenErrorPtr> {
            if (success_body.auth_token.empty()) {
              return base::unexpected(
                  MakeServerError<mojom::GetServiceTokenError>(
                      status_code,
                      mojom::GetServiceTokenServerErrorCode::kInvalidResponse));
            }

            auto encrypted_service_token = Encrypt(success_body.auth_token);
            if (encrypted_service_token.empty()) {
              return base::unexpected(
                  MakeClientError<mojom::GetServiceTokenError>(
                      mojom::GetServiceTokenClientErrorCode::
                          kServiceTokenEncryptionFailed));
            }

            account_state_prefs_->CacheServiceToken(
                service_name, std::move(encrypted_service_token));

            return mojom::GetServiceTokenResult::New(
                std::move(success_body.auth_token));
          });

  std::move(callback).Run(std::move(result));
}

void LoggedInState::ScheduleAuthValidate(
    base::TimeDelta delay,
    RequestHandle current_auth_validate_request) {
  auth_validate_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&LoggedInState::AuthValidate, base::Unretained(this),
                     std::move(current_auth_validate_request)));
}

void LoggedInState::AuthValidate(RequestHandle current_auth_validate_request) {
  current_auth_validate_request.reset();

  const auto encrypted_authentication_token =
      account_state_prefs_->GetAuthenticationToken();
  CHECK(!encrypted_authentication_token.empty());
  const auto authentication_token = Decrypt(encrypted_authentication_token);
  if (authentication_token.empty()) {
    return;
  }

  auto request = MakeRequest<WithHeaders<AuthValidate::Request>>();
  SetBearerToken(request, authentication_token);

  // Not adopted into the state's in-flight bag:
  // the request handle is passed forward into the next scheduled
  // `AuthValidate`, which resets it (cancelling any still-pending previous
  // attempt) before issuing the new one.
  current_auth_validate_request =
      SendCallerOwnedRequest<endpoints::AuthValidate>(
          std::move(request), base::BindOnce(&LoggedInState::OnAuthValidate,
                                             weak_factory_.GetWeakPtr()));

  // Replace normal cadence with the watchdog timer.
  ScheduleAuthValidate(kWatchdogInterval,
                       std::move(current_auth_validate_request));
}

void LoggedInState::OnAuthValidate(AuthValidate::Response response) {
  const auto email =
      response.body
          ? std::move(*response.body)
                .transform([](auto success_body) { return success_body.email; })
                .value_or("")
          : "";

  if (!email.empty()) {
    account_state_prefs_->UpdateEmail(email);
  } else if (response.status_code >= 400 && response.status_code < 500) {
    // Force logged-out (and stop polling) to prevent presenting invalid state
    // to the user and issuing invalid requests.
    //
    // See `StateBase`'s class comment on ordering.
    // LoggedIn ==> LoggedOut (state swap).
    return account_state_prefs_->SetLoggedOut();
  }

  // Replace watchdog timer with the normal cadence.
  ScheduleAuthValidate(kAuthValidatePollInterval);
}

}  // namespace brave_account
