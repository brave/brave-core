/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_

#include <list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_encryption.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/state_internal.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

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

  template <typename Error = void>
  auto GetDecryptedVerificationToken(
      mojom::VerificationIntentPtr intent) const {
    if constexpr (std::is_void_v<Error>) {
      return Decrypt(
          account_state_prefs_->GetVerificationToken(std::move(intent)));
    } else {
      using Expected =
          base::expected<std::string, decltype(Error::NewClientError(nullptr))>;

      const auto encrypted_verification_token =
          account_state_prefs_->GetVerificationToken(std::move(intent));
      if (encrypted_verification_token.empty()) {
        return Expected(internal::MakeCalledInWrongStateError<Error>());
      }

      auto verification_token = Decrypt(encrypted_verification_token);
      if (verification_token.empty()) {
        return Expected(
            internal::MakeVerificationTokenDecryptionFailedError<Error>());
      }

      return Expected(std::move(verification_token));
    }
  }

  template <typename Error = void>
  auto GetDecryptedAuthenticationToken() const {
    const auto encrypted_authentication_token =
        account_state_prefs_->GetAuthenticationToken();
    CHECK(!encrypted_authentication_token.empty());

    auto authentication_token = Decrypt(encrypted_authentication_token);
    if constexpr (std::is_void_v<Error>) {
      return authentication_token;
    } else {
      using Expected =
          base::expected<std::string, decltype(Error::NewClientError(nullptr))>;

      if (authentication_token.empty()) {
        return Expected(
            internal::MakeAuthenticationTokenDecryptionFailedError<Error>());
      }

      return Expected(std::move(authentication_token));
    }
  }

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
  // Flow helpers, each owned by one state and implementing that state's
  // password-flow overrides on its owner's behalf: `ResetPassword` owned by
  // `LoggedOutState` (the `ResetPassword*` overrides), `ChangePassword` owned
  // by `LoggedInState` (the `ChangePassword*` overrides). They are friended on
  // `StateBase` (not on the owning state) because the plumbing they borrow
  // through their back-reference - request lifetime (`in_flight_`,
  // `SendStateOwnedRequest()`), crypto, and `account_state_prefs_` - is bound
  // to `StateBase` and can't move out.
  friend class ChangePassword;
  friend class ResetPassword;

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

  void ResendVerificationEmail(
      mojom::VerificationIntentPtr intent,
      ResendVerificationEmailCallback callback) override;

  void CancelVerification(mojom::VerificationIntentPtr intent) override;

  void ResetPasswordVerifyInit(
      const std::string& email,
      ResetPasswordVerifyInitCallback callback) override;

  void ResetPasswordVerifyComplete(
      const std::string& code,
      ResetPasswordVerifyCompleteCallback callback) override;

  void ResetPasswordPasswordInit(
      const std::string& blinded_message,
      ResetPasswordPasswordInitCallback callback) override;

  void ResetPasswordPasswordFinalize(
      const std::string& serialized_record,
      const std::string& email,
      ResetPasswordPasswordFinalizeCallback callback) override;

  void LoginInitialize(mojom::Service initiating_service,
                       const std::string& email,
                       const std::string& serialized_ke1,
                       LoginInitializeCallback callback) override;

  void LoginFinalize(const std::string& encrypted_login_token,
                     const std::string& client_mac,
                     LoginFinalizeCallback callback) override;

  void ChangePasswordVerifyInit(
      const std::string& email,
      ChangePasswordVerifyInitCallback callback) override;

  void ChangePasswordVerifyComplete(
      const std::string& code,
      ChangePasswordVerifyCompleteCallback callback) override;

  void ChangePasswordPasswordInit(
      const std::string& blinded_message,
      ChangePasswordPasswordInitCallback callback) override;

  void ChangePasswordPasswordFinalize(
      const std::string& serialized_record,
      ChangePasswordPasswordFinalizeCallback callback) override;

  void LogOut() override;

  void GetServiceToken(mojom::Service service,
                       GetServiceTokenCallback callback) override;

  void OnResendVerificationEmail(ResendVerificationEmailCallback callback,
                                 endpoints::VerifyResend::Response response);

  void RemoveRequestHandle(
      std::list<endpoint_client::RequestHandle>::iterator slot);

 protected:
  const raw_ref<AccountStatePrefs> account_state_prefs_;

 private:
  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const BraveAccountEncryption encryption_;
  const AddObserverCallback add_observer_;
  mojo::ReceiverSet<mojom::Authentication> receivers_;
  std::list<endpoint_client::RequestHandle> in_flight_;
  base::WeakPtrFactory<StateBase> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
