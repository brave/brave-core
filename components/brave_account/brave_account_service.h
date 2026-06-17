/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

#include <variant>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/logged_in_state.h"
#include "brave/components/brave_account/logged_out_state.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class OSCryptAsync;
}  // namespace os_crypt_async

namespace brave_account {

// BraveAccountService has no non-Mojom callers. Its only public entrypoint is
// `BindInterface()`, and this should remain the case. Receiver binding is
// deferred until `FinishInitialization()` installs the encryptor, and any
// service-initiated work that can reach encryption is also started only after
// that point.
//
// State-scoped Mojom receivers and network requests:
//
// `state_` is a `std::variant<>` whose active alternative is the current
// account state: `LoggedOutState` or `LoggedInState`. Each state inherits
// `mojom::Authentication` via `StateBase` and owns both its in-flight
// network requests *and* the Mojom receivers currently bound to it. When
// `OnAccountStateChanged()` observes a pref change that crosses variant
// alternatives, the service calls `TakeReceivers()` on the outgoing state,
// replaces `state_` (the outgoing state's destructor cancels its in-flight
// network requests), then re-binds the receivers onto the incoming state.
// In-flight Mojom messages on the pipe survive the migration and are delivered
// to the new state, where wrong-state ones land in `StateBase`'s default
// response.
//
// `AddObserver()` is the one Mojom method that is not state-scoped, since an
// observer's subscription must span state transitions. `StateBase` forwards
// it back to the service via an `AddObserverCallback` injected at state
// construction; `observers_` and `OnAccountStateChanged()` notifications live
// on the service.
class BraveAccountService : public KeyedService {
 public:
  BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      os_crypt_async::OSCryptAsync* os_crypt_async);

  BraveAccountService(const BraveAccountService&) = delete;
  BraveAccountService& operator=(const BraveAccountService&) = delete;

  ~BraveAccountService() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::Authentication> pending_receiver);

  base::OneShotTimer* AuthValidateTimerForTesting();

 private:
  void FinishInitialization(scoped_refptr<os_crypt_async::Encryptor> encryptor);

  void AddObserver(mojo::PendingRemote<mojom::AuthenticationObserver> observer);

  void OnAccountStateChanged();

  void EnsureState(mojom::AccountState::Tag which);

  StateBase& ActiveState();

  AccountStatePrefs account_state_prefs_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  scoped_refptr<os_crypt_async::Encryptor> encryptor_;
  std::vector<mojo::PendingReceiver<mojom::Authentication>> pending_receivers_;
  mojo::RemoteSet<mojom::AuthenticationObserver> observers_;
  std::variant<std::monostate, LoggedOutState, LoggedInState> state_;
  base::WeakPtrFactory<BraveAccountService> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
