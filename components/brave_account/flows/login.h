/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOGIN_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOGIN_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/login_finalize.h"
#include "brave/components/brave_account/endpoints/login_init.h"
#include "brave/components/brave_account/flows/flow_base.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the login half of the logged-out `mojom::Authentication` surface.
// `LoggedOutState` holds a `Login` member and forwards the two mojom
// `LoginStep*` methods to the matching step here. The two steps map
// one-to-one onto backend endpoints:
//
//   Step1 -> /v2/auth/login/init
//   Step2 -> /v2/auth/login/finalize
//
// Requests are sent through the inherited `FlowBase` helpers, so their
// lifetime is tied to this flow (see `FlowBase::SendStateOwnedRequest`).
class Login : public FlowBase {
 public:
  Login(AccountStatePrefs& account_state_prefs,
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        const os_crypt_async::Encryptor& encryptor);

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

  base::WeakPtrFactory<Login> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOGIN_H_
