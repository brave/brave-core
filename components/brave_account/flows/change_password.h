/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CHANGE_PASSWORD_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CHANGE_PASSWORD_H_

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
// four mojom `ChangePasswordStep*` methods to the matching step here. The four
// steps map one-to-one onto backend endpoints:
//
//   Step1 -> /v2/verify/init
//   Step2 -> /v2/verify/complete
//   Step3 -> /v2/accounts/password/init
//   Step4 -> /v2/accounts/password/finalize
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class ChangePassword {
 public:
  explicit ChangePassword(StateBase& state);

  ChangePassword(const ChangePassword&) = delete;
  ChangePassword& operator=(const ChangePassword&) = delete;

  ~ChangePassword();

  void Step1(const std::string& email,
             mojom::Authentication::ChangePasswordStep1Callback callback);

  void Step2(const std::string& code,
             mojom::Authentication::ChangePasswordStep2Callback callback);

  void Step3(const std::string& blinded_message,
             mojom::Authentication::ChangePasswordStep3Callback callback);

  void Step4(const std::string& serialized_record,
             mojom::Authentication::ChangePasswordStep4Callback callback);

 private:
  void OnStep1(mojom::Authentication::ChangePasswordStep1Callback callback,
               endpoints::VerifyInit::Response response);

  void OnStep2(mojom::Authentication::ChangePasswordStep2Callback callback,
               endpoints::VerifyComplete::Response response);

  void OnStep3(mojom::Authentication::ChangePasswordStep3Callback callback,
               endpoints::PasswordInit::Response response);

  void OnStep4(mojom::Authentication::ChangePasswordStep4Callback callback,
               endpoints::PasswordFinalize::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<ChangePassword> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_CHANGE_PASSWORD_H_
