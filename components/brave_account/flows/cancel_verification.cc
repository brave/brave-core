/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/cancel_verification.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/verify_delete.h"
#include "brave/components/brave_account/state_internal.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::VerifyDelete;
using internal::MakeRequest;

CancelVerification::CancelVerification(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor)
    : FlowBase(account_state_prefs, std::move(url_loader_factory), encryptor) {}

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
            GetDecryptedVerificationToken(std::move(intent));
        !verification_token.empty()) {
      auto request = MakeRequest<WithHeaders<VerifyDelete::Request>>();
      SetBearerToken(request, verification_token);

      SendUnownedRequest<VerifyDelete>(std::move(request));
    }
  }

  // LoggedOutWithVerification ==> LoggedOut (no state swap), or
  // LoggedInWithVerification ==> LoggedIn (no state swap).
  account_state_prefs_->ClearVerification();
}

}  // namespace brave_account
