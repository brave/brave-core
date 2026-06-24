/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_CHANGE_PASSWORD_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_CHANGE_PASSWORD_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/verify_complete.h"
#include "brave/components/brave_account/endpoints/verify_init.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace brave_account {

class StateBase;

// Owns the password-change half of the logged-in `mojom::Authentication`
// surface. `LoggedInState` holds a `ChangePassword` member and forwards the
// four mojom `ChangePassword*` methods to the matching method here. The four
// steps map one-to-one onto backend endpoints, and the methods are named
// after them:
//
//   VerifyInit       -> /v2/verify/init
//   VerifyComplete   -> /v2/verify/complete
//   PasswordInit     -> /v2/accounts/password/init
//   PasswordFinalize -> /v2/accounts/password/finalize
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class ChangePassword {
 public:
  explicit ChangePassword(StateBase& state);

  ChangePassword(const ChangePassword&) = delete;
  ChangePassword& operator=(const ChangePassword&) = delete;

  ~ChangePassword();

  void VerifyInit(
      const std::string& email,
      mojom::Authentication::ChangePasswordVerifyInitCallback callback);

  void VerifyComplete(
      const std::string& code,
      mojom::Authentication::ChangePasswordVerifyCompleteCallback callback);

  void PasswordInit(
      const std::string& blinded_message,
      mojom::Authentication::ChangePasswordPasswordInitCallback callback);

  void PasswordFinalize(
      const std::string& serialized_record,
      mojom::Authentication::ChangePasswordPasswordFinalizeCallback callback);

 private:
  void OnVerifyInit(
      mojom::Authentication::ChangePasswordVerifyInitCallback callback,
      endpoints::VerifyInit::Response response);

  void OnVerifyComplete(
      mojom::Authentication::ChangePasswordVerifyCompleteCallback callback,
      endpoints::VerifyComplete::Response response);

  void OnPasswordInit(
      mojom::Authentication::ChangePasswordPasswordInitCallback callback,
      endpoints::PasswordInit::Response response);

  void OnPasswordFinalize(
      mojom::Authentication::ChangePasswordPasswordFinalizeCallback callback,
      endpoints::PasswordFinalize::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<ChangePassword> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_CHANGE_PASSWORD_H_
