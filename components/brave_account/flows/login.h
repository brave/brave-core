/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOGIN_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOGIN_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/endpoints/login_finalize.h"
#include "brave/components/brave_account/endpoints/login_init.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace brave_account {

class StateBase;

// Owns the login half of the logged-out `mojom::Authentication` surface.
// `LoggedOutState` holds a `Login` member and forwards the two mojom
// `LoginStep*` methods to the matching step here. The two steps map
// one-to-one onto backend endpoints:
//
//   Step1 -> /v2/auth/login/init
//   Step2 -> /v2/auth/login/finalize
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class Login {
 public:
  explicit Login(StateBase& state);

  Login(const Login&) = delete;
  Login& operator=(const Login&) = delete;

  ~Login();

  void Step1(mojom::Service initiating_service,
             const std::string& email,
             const std::string& serialized_ke1,
             mojom::Authentication::LoginStep1Callback callback);

  void Step2(const std::string& encrypted_login_token,
             const std::string& client_mac,
             mojom::Authentication::LoginStep2Callback callback);

 private:
  void OnStep1(mojom::Authentication::LoginStep1Callback callback,
               endpoints::LoginInit::Response response);

  void OnStep2(mojom::Authentication::LoginStep2Callback callback,
               endpoints::LoginFinalize::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<Login> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOGIN_H_
