/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/logged_in_state.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/state_internal.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::RequestHandle;
using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::AuthValidate;
using internal::MakeRequest;

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

void LoggedInState::ChangePasswordStep1(const std::string& email,
                                        ChangePasswordStep1Callback callback) {
  change_password_.Step1(email, std::move(callback));
}

void LoggedInState::ChangePasswordStep2(const std::string& code,
                                        ChangePasswordStep2Callback callback) {
  change_password_.Step2(code, std::move(callback));
}

void LoggedInState::ChangePasswordStep3(const std::string& blinded_message,
                                        ChangePasswordStep3Callback callback) {
  change_password_.Step3(blinded_message, std::move(callback));
}

void LoggedInState::ChangePasswordStep4(const std::string& serialized_record,
                                        ChangePasswordStep4Callback callback) {
  change_password_.Step4(serialized_record, std::move(callback));
}

void LoggedInState::LogOut() {
  log_out_();
}

void LoggedInState::GetServiceToken(mojom::Service service,
                                    GetServiceTokenCallback callback) {
  get_service_token_(service, std::move(callback));
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

  const auto authentication_token = GetDecryptedAuthenticationToken();
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
