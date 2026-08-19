/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CANCEL_VERIFICATION_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CANCEL_VERIFICATION_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace brave_account {

class StateBase;

// Owns the verification cancellation of the `mojom::Authentication` surface.
// Like `ResendVerificationEmail`, this one is held by `StateBase` itself, since
// `CancelVerification()` is valid in both the logged-out and logged-in states:
// `StateBase` holds a `CancelVerification` member and forwards the single mojom
// `CancelVerification` method to `operator()()` here, which best-effort
// notifies one backend endpoint before dropping the verification slot:
//
//   operator()() -> /v2/verify/delete
//
// The request is sent through the owning state's `StateBase` helpers, so its
// lifetime is tied to that state (see `StateBase::SendUnownedRequest`).
class CancelVerification {
 public:
  explicit CancelVerification(StateBase& state);

  CancelVerification(const CancelVerification&) = delete;
  CancelVerification& operator=(const CancelVerification&) = delete;

  ~CancelVerification();

  void operator()(mojom::VerificationIntentPtr intent);

 private:
  const raw_ref<StateBase> state_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CANCEL_VERIFICATION_H_
