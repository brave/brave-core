/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/log_out.h"

#include <utility>

#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/auth_logout.h"
#include "brave/components/brave_account/state_base.h"
#include "brave/components/brave_account/state_internal.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::AuthLogout;
using internal::MakeRequest;

LogOut::LogOut(StateBase& state) : state_(state) {}

LogOut::~LogOut() = default;

void LogOut::operator()() {
  // Best-effort notification to the server, since server side will clean up
  // authentication tokens automatically (currently in 6 months of inactivity).
  // Not adopted into the state's in-flight bag:
  // best-effort with no callback that touches state.
  if (const auto authentication_token =
          state_->GetDecryptedAuthenticationToken();
      !authentication_token.empty()) {
    auto request = MakeRequest<WithHeaders<AuthLogout::Request>>();
    SetBearerToken(request, authentication_token);

    state_->SendUnownedRequest<AuthLogout>(std::move(request));
  }

  // See `StateBase`'s class comment on ordering.
  // LoggedIn ==> LoggedOut (state swap).
  state_->account_state_prefs_->SetLoggedOut();
}

}  // namespace brave_account
