/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/update_email.h"

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

UpdateEmail::UpdateEmail(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor)
    : FlowBase(account_state_prefs, std::move(url_loader_factory), encryptor) {
  ScheduleRequest();
}

UpdateEmail::~UpdateEmail() = default;

void UpdateEmail::ScheduleRequest(base::TimeDelta delay,
                                  RequestHandle current_request) {
  timer_.Start(FROM_HERE, delay,
               base::BindOnce(&UpdateEmail::SendRequest, base::Unretained(this),
                              std::move(current_request)));
}

void UpdateEmail::SendRequest(RequestHandle current_request) {
  current_request.reset();

  const auto authentication_token = GetDecryptedAuthenticationToken();
  if (authentication_token.empty()) {
    return;
  }

  auto request = MakeRequest<WithHeaders<AuthValidate::Request>>();
  SetBearerToken(request, authentication_token);

  // Not adopted into the state's in-flight bag:
  // the request handle is passed forward into the next scheduled
  // `SendRequest`, which resets it (cancelling any still-pending previous
  // attempt) before issuing the new one.
  current_request = SendCallerOwnedRequest<AuthValidate>(
      std::move(request),
      base::BindOnce(&UpdateEmail::OnResponse, weak_factory_.GetWeakPtr()));

  // Replace normal cadence with the watchdog timer.
  ScheduleRequest(kWatchdogInterval, std::move(current_request));
}

void UpdateEmail::OnResponse(AuthValidate::Response response) {
  const auto email = response.body ? std::move(*response.body)
                                         .transform([](auto success_body) {
                                           return std::move(success_body.email);
                                         })
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
  ScheduleRequest(kAuthValidatePollInterval);
}

}  // namespace brave_account
