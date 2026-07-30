/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_REGISTER_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_REGISTER_H_

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
// three mojom `RegisterStep*` methods to the matching step here. The three
// steps map one-to-one onto backend endpoints:
//
//   Step1 -> /v2/accounts/password/init
//   Step2 -> /v2/accounts/password/finalize
//   Step3 -> /v2/verify/complete
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class Register {
 public:
  explicit Register(StateBase& state);

  Register(const Register&) = delete;
  Register& operator=(const Register&) = delete;

  ~Register();

  void Step1(mojom::Service initiating_service,
             const std::string& email,
             const std::string& blinded_message,
             mojom::Authentication::RegisterStep1Callback callback);

  void Step2(const std::string& encrypted_verification_token,
             const std::string& serialized_record,
             mojom::Authentication::RegisterStep2Callback callback);

  void Step3(const std::string& code,
             mojom::Authentication::RegisterStep3Callback callback);

 private:
  void OnStep1(mojom::Authentication::RegisterStep1Callback callback,
               endpoints::PasswordInit::Response response);

  void OnStep2(mojom::Authentication::RegisterStep2Callback callback,
               const std::string& encrypted_verification_token,
               endpoints::PasswordFinalize::Response response);

  void OnStep3(mojom::Authentication::RegisterStep3Callback callback,
               endpoints::VerifyComplete::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<Register> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_REGISTER_H_
