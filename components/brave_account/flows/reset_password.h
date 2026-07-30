/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESET_PASSWORD_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESET_PASSWORD_H_

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
// four mojom `ResetPasswordStep*` methods to the matching step here. The four
// steps map one-to-one onto backend endpoints:
//
//   Step1 -> /v2/verify/init
//   Step2 -> /v2/verify/complete
//   Step3 -> /v2/accounts/password/init
//   Step4 -> /v2/accounts/password/finalize
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class ResetPassword {
 public:
  explicit ResetPassword(StateBase& state);

  ResetPassword(const ResetPassword&) = delete;
  ResetPassword& operator=(const ResetPassword&) = delete;

  ~ResetPassword();

  void Step1(const std::string& email,
             mojom::Authentication::ResetPasswordStep1Callback callback);

  void Step2(const std::string& code,
             mojom::Authentication::ResetPasswordStep2Callback callback);

  void Step3(const std::string& blinded_message,
             mojom::Authentication::ResetPasswordStep3Callback callback);

  void Step4(const std::string& serialized_record,
             const std::string& email,
             mojom::Authentication::ResetPasswordStep4Callback callback);

 private:
  void OnStep1(mojom::Authentication::ResetPasswordStep1Callback callback,
               endpoints::VerifyInit::Response response);

  void OnStep2(mojom::Authentication::ResetPasswordStep2Callback callback,
               endpoints::VerifyComplete::Response response);

  void OnStep3(mojom::Authentication::ResetPasswordStep3Callback callback,
               endpoints::PasswordInit::Response response);

  void OnStep4(mojom::Authentication::ResetPasswordStep4Callback callback,
               const std::string& email,
               endpoints::PasswordFinalize::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<ResetPassword> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_RESET_PASSWORD_H_
