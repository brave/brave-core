/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_REGISTER_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_REGISTER_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/verify_complete.h"
#include "brave/components/brave_account/flows/flow_base.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the registration half of the logged-out `mojom::Authentication`
// surface. `LoggedOutState` holds a `Register` member and forwards the
// three mojom `RegisterStep*` methods to the matching step here. The three
// steps map one-to-one onto backend endpoints:
//
//   Step1 -> /v2/accounts/password/init
//   Step2 -> /v2/accounts/password/finalize
//   Step3 -> /v2/verify/complete
//
// Requests are sent through the inherited `FlowBase` helpers, so their
// lifetime is tied to this flow (see `FlowBase::SendStateOwnedRequest`).
class Register : public FlowBase {
 public:
  Register(AccountStatePrefs& account_state_prefs,
           scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
           const os_crypt_async::Encryptor& encryptor);

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

  base::WeakPtrFactory<Register> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_REGISTER_H_
