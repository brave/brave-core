/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_REGISTER_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_REGISTER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/verify_complete.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace brave_account {

class StateBase;

// Owns the registration half of the logged-out `mojom::Authentication`
// surface. `LoggedOutState` holds a `Register` member and forwards the
// three mojom `Register*` methods to the matching method here. The three
// steps map one-to-one onto backend endpoints, and the methods are named
// after them:
//
//   PasswordInit     -> /v2/accounts/password/init
//   PasswordFinalize -> /v2/accounts/password/finalize
//   VerifyComplete   -> /v2/verify/complete
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class Register {
 public:
  explicit Register(StateBase& state);

  Register(const Register&) = delete;
  Register& operator=(const Register&) = delete;

  ~Register();

  void PasswordInit(
      mojom::Service initiating_service,
      const std::string& email,
      const std::string& blinded_message,
      mojom::Authentication::RegisterPasswordInitCallback callback);

  void PasswordFinalize(
      const std::string& encrypted_verification_token,
      const std::string& serialized_record,
      mojom::Authentication::RegisterPasswordFinalizeCallback callback);

  void VerifyComplete(
      const std::string& code,
      mojom::Authentication::RegisterVerifyCompleteCallback callback);

 private:
  void OnPasswordInit(
      mojom::Authentication::RegisterPasswordInitCallback callback,
      endpoints::PasswordInit::Response response);

  void OnPasswordFinalize(
      mojom::Authentication::RegisterPasswordFinalizeCallback callback,
      const std::string& encrypted_verification_token,
      endpoints::PasswordFinalize::Response response);

  void OnVerifyComplete(
      mojom::Authentication::RegisterVerifyCompleteCallback callback,
      endpoints::VerifyComplete::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<Register> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_REGISTER_H_
