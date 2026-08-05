/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOG_OUT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOG_OUT_H_

#include "base/memory/raw_ref.h"

namespace brave_account {

class StateBase;

// Owns the log-out of the logged-in `mojom::Authentication` surface.
// `LoggedInState` holds a `LogOut` member and forwards the single mojom
// `LogOut` method to `operator()()` here, which best-effort notifies one
// backend endpoint before swapping the state to logged-out:
//
//   operator()() -> /v2/auth/logout
//
// The request is sent through the owning state's `StateBase` helpers, so its
// lifetime is tied to that state (see `StateBase::SendUnownedRequest`).
class LogOut {
 public:
  explicit LogOut(StateBase& state);

  LogOut(const LogOut&) = delete;
  LogOut& operator=(const LogOut&) = delete;

  ~LogOut();

  void operator()();

 private:
  const raw_ref<StateBase> state_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOG_OUT_H_
