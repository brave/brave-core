/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESEND_VERIFICATION_EMAIL_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESEND_VERIFICATION_EMAIL_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace brave_account {

class StateBase;

// Owns the verification-email resend of the `mojom::Authentication` surface.
// Unlike the other flow helpers, this one is held by `StateBase` itself, since
// `ResendVerificationEmail()` is valid in both the logged-out and logged-in
// states: `StateBase` holds a `ResendVerificationEmail` member and forwards
// the single mojom `ResendVerificationEmail` method to `operator()()` here,
// which maps onto one backend endpoint:
//
//   operator()() -> /v2/verify/resend
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class ResendVerificationEmail {
 public:
  explicit ResendVerificationEmail(StateBase& state);

  ResendVerificationEmail(const ResendVerificationEmail&) = delete;
  ResendVerificationEmail& operator=(const ResendVerificationEmail&) = delete;

  ~ResendVerificationEmail();

  void operator()(
      mojom::VerificationIntentPtr intent,
      mojom::Authentication::ResendVerificationEmailCallback callback);

 private:
  void OnResponse(
      mojom::Authentication::ResendVerificationEmailCallback callback,
      endpoints::VerifyResend::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<ResendVerificationEmail> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESEND_VERIFICATION_EMAIL_H_
