/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_DELETE_ACCOUNT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_DELETE_ACCOUNT_H_

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/accounts_delete.h"
#include "brave/components/brave_account/flows/flow_base.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the account deletion of the logged-in `mojom::Authentication` surface.
// `LoggedInState` holds a `DeleteAccount` member and forwards the mojom
// `DeleteAccount` method to `operator()()` here:
//
//   operator()() -> /v2/accounts
//
// Requests are sent through the inherited `FlowBase` helpers, so their
// lifetime is tied to this flow (see `FlowBase::SendFlowOwnedRequest`).
class DeleteAccount : public FlowBase {
 public:
  DeleteAccount(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor);

  DeleteAccount(const DeleteAccount&) = delete;
  DeleteAccount& operator=(const DeleteAccount&) = delete;

  ~DeleteAccount();

  void operator()(mojom::Authentication::DeleteAccountCallback callback);

 private:
  void OnResponse(mojom::Authentication::DeleteAccountCallback callback,
                  endpoints::AccountsDelete::Response response);

  base::WeakPtrFactory<DeleteAccount> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_DELETE_ACCOUNT_H_
