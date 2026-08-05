/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_GET_SERVICE_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_GET_SERVICE_TOKEN_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/service_token.h"
#include "brave/components/brave_account/flows/flow_base.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the service-token half of the logged-in `mojom::Authentication`
// surface. `LoggedInState` holds a `GetServiceToken` member and forwards the
// single mojom `GetServiceToken` method to `operator()()` here, which serves
// the token from the prefs cache when a fresh entry is available, otherwise
// fetches it from one backend endpoint:
//
//   operator()() -> /v2/auth/service_token
//
// Requests are sent through the inherited `FlowBase` helpers, so their
// lifetime is tied to this flow (see `FlowBase::SendStateOwnedRequest`).
class GetServiceToken : public FlowBase {
 public:
  GetServiceToken(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor);

  GetServiceToken(const GetServiceToken&) = delete;
  GetServiceToken& operator=(const GetServiceToken&) = delete;

  ~GetServiceToken();

  void operator()(mojom::Service service,
                  mojom::Authentication::GetServiceTokenCallback callback);

 private:
  void OnResponse(const std::string& service_name,
                  mojom::Authentication::GetServiceTokenCallback callback,
                  endpoints::ServiceToken::Response response);

  base::WeakPtrFactory<GetServiceToken> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_GET_SERVICE_TOKEN_H_
