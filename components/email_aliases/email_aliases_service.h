/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/elapsed_timer.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_auth.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace os_crypt_async {
class Encryptor;
class OSCryptAsync;
}  // namespace os_crypt_async

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace email_aliases {

// The EmailAliasesService is responsible for managing the email aliases for a
// user. It is used to request authentication, generate aliases, update aliases,
// and delete aliases. It also provides a way to observe the authentication
// state of the user. The service is designed to be used in a multi-profile
// environment, where each profile has its own EmailAliasesService instance.
//
// The service is used by the EmailAliases UI to respond to user actions.
class EmailAliasesService : public KeyedService,
                            public mojom::EmailAliasesService {
 public:
  EmailAliasesService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* pref_service,
      os_crypt_async::OSCryptAsync* os_crypt_async);
  ~EmailAliasesService() override;

  static void RegisterProfilePref(PrefRegistrySimple* registry);

  // KeyedService:
  // Called when the owning profile context is shutting down. Releases
  // interface bindings and observers.
  void Shutdown() override;

  // mojom::EmailAliasesService:
  // Initiates the authentication flow for the provided |auth_email|. On success
  // the service transitions to kAuthenticating and continues with session
  // polling.
  void RequestAuthentication(const std::string& auth_email,
                             RequestAuthenticationCallback callback) override;

  // Cancels any in-flight verification requests, clears authentication state,
  // transitions to kUnauthenticated, and acknowledges via |callback|.
  void CancelAuthenticationOrLogout(
      CancelAuthenticationOrLogoutCallback callback) override;

  // Requests generation of a new alias and returns the result via |callback|.
  void GenerateAlias(GenerateAliasCallback callback) override;

  // Creates or updates an alias identified by |alias_email| with optional note.
  void UpdateAlias(const std::string& alias_email,
                   const std::optional<std::string>& note,
                   UpdateAliasCallback callback) override;

  // Deletes the alias identified by |alias_email|.
  void DeleteAlias(const std::string& alias_email,
                   DeleteAliasCallback callback) override;

  // Registers |observer| to receive authentication state updates. The observer
  // will immediately receive the current state upon registration.
  void AddObserver(mojo::PendingRemote<mojom::EmailAliasesServiceObserver>
                       observer) override;

  // Returns true if the user is authenticated.
  bool IsAuthenticated() const;

  // Binds the mojom interface to this service
  // Adds a new receiver for the EmailAliasesService Mojo interface.
  void BindInterface(
      mojo::PendingReceiver<mojom::EmailAliasesService> receiver);

  // Returns the current auth token for tests. Empty when unauthenticated.
  std::string GetAuthTokenForTesting();

  // Build the fully-qualified Brave Accounts verification URLs.
  static GURL GetAccountsServiceVerifyInitURL();
  static GURL GetAccountsServiceVerifyResultURL();

  // Returns the base URL for the Email Aliases service.
  static GURL GetEmailAliasesServiceURL();

 private:
  // Callback that receives the response body as an optional string.
  using BodyAsStringCallback =
      base::OnceCallback<void(std::optional<std::string> response_body)>;

  void OnEncryptorReady(os_crypt_async::Encryptor encryptor);

  std::string GetAuthEmail() const;
  std::string GetAuthToken();

  mojom::AuthenticationStatus GetCurrentStatus();

  void OnAuthChanged();

  // Handles the response to the verify/init request. Parses a verification
  // token and, if present, proceeds to poll the session endpoint. Invokes
  // |callback| with an optional error message.
  void OnRequestAuthenticationResponse(
      RequestAuthenticationCallback callback,
      std::optional<std::string> response_body);

  // Posts a request to the verify/result endpoint to wait for completion of
  // the authentication flow.
  void RequestSession();

  // Handles the response to the verify/result polling request. Extracts the
  // auth token and transitions to kAuthenticated, or calls
  // MaybeRequestSessionAgain when authentication is still pending.
  void OnRequestSessionResponse(std::optional<std::string> response_body);

  // Decides whether to call RequestSession again, depending on the whether
  // the polling window has elapsed. Adds a delay if the minimum interval
  // between requests has not yet elapsed.
  void MaybeRequestSessionAgain();

  // Notifies all registered observers of an authentication state change.
  void NotifyObserversAuthStateChanged(
      mojom::AuthenticationStatus status,
      const std::optional<std::string>& error_message = std::nullopt);

  // Cancels in-flight verification requests and clears verification/auth
  // tokens to reset the authentication flow to a clean state.
  void ResetVerificationFlow();

  // Fetch helper for Email Aliases backend. Specifically for GET/HEAD.
  void ApiFetch(const GURL& url,
                const std::string_view method,
                BodyAsStringCallback download_to_string_callback);

  // Fetch helper which uploads |body_value|. Specifically for POST/PUT/DELETE.
  void ApiFetch(const GURL& url,
                const std::string_view method,
                const base::Value::Dict& body_value,
                BodyAsStringCallback download_to_string_callback);

  // Shared implementation used by the two ApiFetch overloads to make a network
  // request.
  void ApiFetchInternal(const GURL& url,
                        const std::string_view method,
                        std::optional<std::string> serialized_body,
                        BodyAsStringCallback download_to_string_callback);

  // Refreshes the aliases list from the server and notifies observers.
  void RefreshAliases();

  // Parses and applies the aliases list received from the backend.
  void OnRefreshAliasesResponse(std::optional<std::string> response_body);

  // Processes the server response for a generate-alias request.
  void OnGenerateAliasResponse(GenerateAliasCallback user_callback,
                               std::optional<std::string> response_body);

  // Common handler for alias edit responses (update/delete).
  void OnEditAliasResponse(
      base::OnceCallback<void(base::expected<std::monostate, std::string>)>
          user_callback,
      bool update_expected,
      std::optional<std::string> response_body);

  // Bound Mojo receivers for the EmailAliasesService interface.
  mojo::ReceiverSet<mojom::EmailAliasesService> receivers_;

  // Connected observers that receive authentication state updates.
  mojo::RemoteSet<mojom::EmailAliasesServiceObserver> observers_;

  // Temporary token returned by verify/init and used to authorize polling.
  std::string verification_token_;

  std::optional<EmailAliasesAuth> auth_;

  // URL loader factory used to issue network requests to Brave Accounts.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  const raw_ptr<PrefService> pref_service_ = nullptr;

  // Single SimpleURLLoader instance used for both verify/init and
  // verify/result requests. Recreated for each new request.
  std::unique_ptr<network::SimpleURLLoader> verification_simple_url_loader_;

  // Cached URLs computed once per service lifetime
  // Cached fully-qualified verify/init URLs.
  const GURL verify_init_url_;

  // Cached fully-qualified verify/result URL.
  const GURL verify_result_url_;

  // Cached fully-qualified email aliases service base URL.
  const GURL email_aliases_service_base_url_;

  // One-shot timer used to delay subsequent verify/result polls so that they
  // are not issued more frequently than the minimum interval.
  base::OneShotTimer session_request_timer_;

  // Elapsed timer for the current verification polling window. Used to
  // enforce a maximum total polling duration.
  std::optional<base::ElapsedTimer> session_poll_elapsed_timer_;

  // WeakPtrFactory to safely bind callbacks across async network operations.
  base::WeakPtrFactory<EmailAliasesService> weak_factory_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
