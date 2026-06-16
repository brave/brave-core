/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_RESET_PASSWORD_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_RESET_PASSWORD_H_

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

// Owns the password-reset half of the logged-out `mojom::Authentication`
// surface. `LoggedOutState` holds a `ResetPassword` member and forwards the
// four mojom `ResetPassword*` methods to the matching method here. The four
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
class ResetPassword {
 public:
  explicit ResetPassword(StateBase& state);

  ResetPassword(const ResetPassword&) = delete;
  ResetPassword& operator=(const ResetPassword&) = delete;

  ~ResetPassword();

  void VerifyInit(
      const std::string& email,
      mojom::Authentication::ResetPasswordVerifyInitCallback callback);

  void VerifyComplete(
      const std::string& code,
      mojom::Authentication::ResetPasswordVerifyCompleteCallback callback);

  void PasswordInit(
      const std::string& blinded_message,
      mojom::Authentication::ResetPasswordPasswordInitCallback callback);

  void PasswordFinalize(
      const std::string& serialized_record,
      const std::string& email,
      mojom::Authentication::ResetPasswordPasswordFinalizeCallback callback);

 private:
  void OnVerifyInit(
      mojom::Authentication::ResetPasswordVerifyInitCallback callback,
      endpoints::VerifyInit::Response response);

  void OnVerifyComplete(
      mojom::Authentication::ResetPasswordVerifyCompleteCallback callback,
      endpoints::VerifyComplete::Response response);

  void OnPasswordInit(
      mojom::Authentication::ResetPasswordPasswordInitCallback callback,
      endpoints::PasswordInit::Response response);

  void OnPasswordFinalize(
      mojom::Authentication::ResetPasswordPasswordFinalizeCallback callback,
      const std::string& email,
      endpoints::PasswordFinalize::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<ResetPassword> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_RESET_PASSWORD_H_
