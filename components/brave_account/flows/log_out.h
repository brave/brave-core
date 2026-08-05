/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOG_OUT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOG_OUT_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/flows/flow_base.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the log-out of the logged-in `mojom::Authentication` surface.
// `LoggedInState` holds a `LogOut` member and forwards the single mojom
// `LogOut` method to `operator()()` here, which best-effort notifies one
// backend endpoint before swapping the state to logged-out:
//
//   operator()() -> /v2/auth/logout
//
// The request is sent through the inherited `FlowBase` helpers, so its
// lifetime is tied to this flow (see `FlowBase::SendUnownedRequest`).
class LogOut : public FlowBase {
 public:
  LogOut(AccountStatePrefs& account_state_prefs,
         scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
         const os_crypt_async::Encryptor& encryptor);

  LogOut(const LogOut&) = delete;
  LogOut& operator=(const LogOut&) = delete;

  ~LogOut();

  void operator()();
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_LOG_OUT_H_
