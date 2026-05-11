/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

// `StateBase` implements `mojom::Authentication`. Derived states override
// only the methods valid in their state; the rest inherit the default
// wrong-state response (client error with `kCalledInWrongState`).
//
// `AddObserver()` is the one method that is not state-scoped; `StateBase`
// forwards it to the service via `AddObserverCallback` so observer
// subscriptions outlive any single state.
//
// Rules for state method implementers:
//
//   - State-mutating pref writes must be the *last* thing a method does. A
//     pref write re-enters `OnAccountStateChanged()` synchronously, and
//     `EnsureState()` unbinds this state's receivers via `TakeReceivers()`
//     (and, when the write crosses variant alternatives, destroys `this`).
//   - Callback-bearing methods must invoke `callback` *before* the pref
//     write; otherwise the response sender held by `callback` becomes
//     invalid when receivers unbind (see `mojo::Receiver::Unbind()` in
//     receiver.h). The rule applies uniformly even for same-alternative
//     writes - the `Authentication` response and `AuthenticationObserver`
//     notifications are on separate pipes, so their relative arrival order
//     at the renderer was never guaranteed in the first place.
//   - Response callbacks bound through `SendStateOwnedRequest()` must use
//     `weak_factory_.GetWeakPtr()`. `~StateBase` cancels `in_flight_`
//     loaders via `DeleteSoon()`, which is not synchronous: a response
//     already queued on the response task runner can still fire the
//     callback after this state has been replaced. The `WeakPtr` drops it.
//
// Two `WeakPtr` factories are involved: the derived state's `weak_factory_`
// invalidates the user callback, and `StateBase::weak_factory_` invalidates
// the internal slot-erase bind. Both are declared last so they destruct
// first.
class StateBase : public mojom::Authentication {
 public:
  StateBase(const StateBase&) = delete;
  StateBase& operator=(const StateBase&) = delete;

  void AddReceiver(mojo::PendingReceiver<mojom::Authentication> receiver);

  std::vector<mojo::PendingReceiver<mojom::Authentication>> TakeReceivers();

 protected:
  using AddObserverCallback = base::RepeatingCallback<void(
      mojo::PendingRemote<mojom::AuthenticationObserver>)>;

  StateBase(AccountStatePrefs& account_state_prefs,
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
            const os_crypt_async::Encryptor& encryptor,
            AddObserverCallback add_observer);

  ~StateBase() override;

  std::string Encrypt(const std::string& plain_text) const;
  std::string Decrypt(const std::string& base64) const;

  // Caller-owned: the returned handle cancels the request when destroyed.
  // Use when the request's lifetime is tied to something other than
  // `~StateBase` (e.g. replaced by the next scheduled attempt).
  template <typename Endpoint, typename Request, typename Response>
  [[nodiscard]] endpoint_client::RequestHandle SendCallerOwnedRequest(
      Request request,
      base::OnceCallback<void(Response)> callback) {
    return endpoint_client::Client<Endpoint>::template Send<
        endpoint_client::RequestCancelability::kCancelable>(
        url_loader_factory_, std::move(request), std::move(callback));
  }

  // State-owned: the handle is parked in `in_flight_` and cancelled by
  // `~StateBase`. The default for response-driven flows.
  template <typename Endpoint, typename Request, typename Response>
  void SendStateOwnedRequest(Request request,
                             base::OnceCallback<void(Response)> callback) {
    auto slot = in_flight_.emplace(in_flight_.end());
    *slot = SendCallerOwnedRequest<Endpoint>(
        std::move(request), std::move(callback).Then(base::BindOnce(
                                &StateBase::RemoveRequestHandle,
                                weak_factory_.GetWeakPtr(), slot)));
  }

  // Unowned: fire-and-forget, no callback, not cancelable. Use only for
  // best-effort notifications whose response is intentionally ignored.
  template <typename Endpoint, typename Request>
  void SendUnownedRequest(Request request) {
    endpoint_client::Client<Endpoint>::Send(
        url_loader_factory_, std::move(request),
        base::BindOnce([](typename Endpoint::Response) {}));
  }

 private:
  void AddObserver(
      mojo::PendingRemote<mojom::AuthenticationObserver> observer) final;

  void RegisterInitialize(mojom::Service initiating_service,
                          const std::string& email,
                          const std::string& blinded_message,
                          RegisterInitializeCallback callback) override;

  void RegisterFinalize(const std::string& encrypted_verification_token,
                        const std::string& serialized_record,
                        RegisterFinalizeCallback callback) override;

  void RegisterVerify(const std::string& code,
                      RegisterVerifyCallback callback) override;

  void ResendConfirmationEmail(
      ResendConfirmationEmailCallback callback) override;

  void CancelRegistration() override;

  void LoginInitialize(mojom::Service initiating_service,
                       const std::string& email,
                       const std::string& serialized_ke1,
                       LoginInitializeCallback callback) override;

  void LoginFinalize(const std::string& encrypted_login_token,
                     const std::string& client_mac,
                     LoginFinalizeCallback callback) override;

  void LogOut() override;

  void GetServiceToken(mojom::Service service,
                       GetServiceTokenCallback callback) override;

  void RemoveRequestHandle(
      std::list<endpoint_client::RequestHandle>::iterator slot);

 protected:
  const raw_ref<AccountStatePrefs> account_state_prefs_;

 private:
  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const raw_ref<const os_crypt_async::Encryptor> encryptor_;
  const AddObserverCallback add_observer_;
  mojo::ReceiverSet<mojom::Authentication> receivers_;
  std::list<endpoint_client::RequestHandle> in_flight_;
  base::WeakPtrFactory<StateBase> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
