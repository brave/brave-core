/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_FLOW_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_FLOW_BASE_H_

#include <list>
#include <string>
#include <type_traits>
#include <utility>

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
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/state_internal.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Shared plumbing for the `mojom::Authentication` flow helpers. Each flow
// (`Login`, `Register`, `LogOut`, ...) derives `FlowBase` and reaches the
// crypto, prefs, and request-sending helpers below by inheritance - no
// back-reference to any state and no friendship required.
//
// Request lifetime: state-owned requests are parked in `in_flight_` and
// cancelled by `~FlowBase`. Because a flow is a member of its owning state,
// the state's destruction destroys the flow, which cancels the flow's pending
// requests. `in_flight_` and the `weak_factory_` that guards its slot-erase
// callbacks live on the same object, so a response still queued when the flow
// is destroyed has its slot-erase dropped by the `WeakPtr` (`~FlowBase`
// cancels loaders via `DeleteSoon()`, which is not synchronous). Response
// callbacks routed through `SendStateOwnedRequest()` must additionally be
// bound through the derived flow's own `WeakPtr` so the user callback is
// dropped too.
class FlowBase {
 public:
  FlowBase(const FlowBase&) = delete;
  FlowBase& operator=(const FlowBase&) = delete;

 protected:
  FlowBase(AccountStatePrefs& account_state_prefs,
           scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
           const os_crypt_async::Encryptor& encryptor);

  ~FlowBase();

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
  // `~FlowBase` (e.g. replaced by the next scheduled attempt).
  template <typename Endpoint, typename Request, typename Response>
  [[nodiscard]] endpoint_client::RequestHandle SendCallerOwnedRequest(
      Request request,
      base::OnceCallback<void(Response)> callback) {
    return endpoint_client::Client<Endpoint>::template Send<
        endpoint_client::RequestCancelability::kCancelable>(
        url_loader_factory_, std::move(request), std::move(callback));
  }

  // State-owned: the handle is parked in `in_flight_` and cancelled by
  // `~FlowBase`. The default for response-driven flows.
  template <typename Endpoint, typename Request, typename Response>
  void SendStateOwnedRequest(Request request,
                             base::OnceCallback<void(Response)> callback) {
    auto slot = in_flight_.emplace(in_flight_.end());
    *slot = SendCallerOwnedRequest<Endpoint>(
        std::move(request),
        std::move(callback).Then(base::BindOnce(
            &FlowBase::RemoveRequestHandle, weak_factory_.GetWeakPtr(), slot)));
  }

  // Unowned: fire-and-forget, no callback, not cancelable. Use only for
  // best-effort notifications whose response is intentionally ignored.
  template <typename Endpoint, typename Request>
  void SendUnownedRequest(Request request) {
    endpoint_client::Client<Endpoint>::Send(
        url_loader_factory_, std::move(request),
        base::BindOnce([](typename Endpoint::Response) {}));
  }

  const raw_ref<AccountStatePrefs> account_state_prefs_;

 private:
  void RemoveRequestHandle(
      std::list<endpoint_client::RequestHandle>::iterator slot);

  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const BraveAccountEncryption encryption_;
  std::list<endpoint_client::RequestHandle> in_flight_;
  base::WeakPtrFactory<FlowBase> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_FLOW_BASE_H_
