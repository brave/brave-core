/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_GET_SERVICE_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_GET_SERVICE_TOKEN_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/endpoints/service_token.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace brave_account {

class StateBase;

// Owns the service-token half of the logged-in `mojom::Authentication`
// surface. `LoggedInState` holds a `GetServiceToken` member and forwards the
// single mojom `GetServiceToken` method to `operator()()` here, which serves
// the token from the prefs cache when a fresh entry is available, otherwise
// fetches it from one backend endpoint:
//
//   operator()() -> /v2/auth/service_token
//
// Requests are sent through the owning state's `StateBase` helpers, so their
// lifetime is tied to that state (see `StateBase::SendStateOwnedRequest`).
class GetServiceToken {
 public:
  explicit GetServiceToken(StateBase& state);

  GetServiceToken(const GetServiceToken&) = delete;
  GetServiceToken& operator=(const GetServiceToken&) = delete;

  ~GetServiceToken();

  void operator()(mojom::Service service,
                  mojom::Authentication::GetServiceTokenCallback callback);

 private:
  void OnResponse(const std::string& service_name,
                  mojom::Authentication::GetServiceTokenCallback callback,
                  endpoints::ServiceToken::Response response);

  const raw_ref<StateBase> state_;

  base::WeakPtrFactory<GetServiceToken> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_GET_SERVICE_TOKEN_H_
