/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/cancel_verification.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/verify_delete.h"
#include "brave/components/brave_account/state_base.h"
#include "brave/components/brave_account/state_internal.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::VerifyDelete;
using internal::MakeRequest;

CancelVerification::CancelVerification(StateBase& state) : state_(state) {}

CancelVerification::~CancelVerification() = default;

void CancelVerification::operator()(mojom::VerificationIntentPtr intent) {
  CHECK(intent);

  if (intent->is_logged_out_intent() &&
      intent->get_logged_out_intent() ==
          mojom::LoggedOutVerificationIntent::kRegistration) {
    // Best-effort notification to the server, since server side will clean up
    // verification tokens automatically (currently after 30 minutes).
    // Not adopted into the state's in-flight bag:
    // best-effort with no callback that touches state.
    if (const auto verification_token =
            state_->GetDecryptedVerificationToken(std::move(intent));
        !verification_token.empty()) {
      auto request = MakeRequest<WithHeaders<VerifyDelete::Request>>();
      SetBearerToken(request, verification_token);

      state_->SendUnownedRequest<VerifyDelete>(std::move(request));
    }
  }

  // LoggedOutWithVerification ==> LoggedOut (no state swap), or
  // LoggedInWithVerification ==> LoggedIn (no state swap).
  state_->account_state_prefs_->ClearVerification();
}

}  // namespace brave_account
