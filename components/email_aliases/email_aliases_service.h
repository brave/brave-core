/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_auth.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "url/gurl.h"

namespace os_crypt_async {
class Encryptor;
class OSCryptAsync;
}  // namespace os_crypt_async

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {
class BraveAccountService;
}

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
      brave_account::BraveAccountService* brave_account_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* pref_service,
      os_crypt_async::OSCryptAsync* os_crypt_async);
  ~EmailAliasesService() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // KeyedService:
  // Called when the owning profile context is shutting down. Releases
  // interface bindings and observers.
  void Shutdown() override;

  // mojom::EmailAliasesService:
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

  // Returns the base URL for the Email Aliases service.
  static GURL GetEmailAliasesServiceURL();

 private:
  // Callback that receives the response body as an optional string.
  using BodyAsStringCallback =
      base::OnceCallback<void(std::optional<std::string> response_body)>;

  void OnEncryptorReady(os_crypt_async::Encryptor encryptor);

  void OnServiceToken(
      base::expected<brave_account::mojom::GetServiceTokenResultPtr,
                     brave_account::mojom::GetServiceTokenErrorPtr> result);

  std::string GetAuthEmail() const;
  std::string GetServiceToken();

  mojom::AuthenticationStatus GetCurrentStatus();

  void OnAuthChanged();

  // Notifies all registered observers of an authentication state change.
  void NotifyObserversAuthStateChanged(mojom::AuthenticationStatus status);

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

  std::optional<EmailAliasesAuth> auth_;
  std::string service_token_;

  const raw_ptr<brave_account::BraveAccountService> brave_account_service_ =
      nullptr;

  // URL loader factory used to issue network requests to Brave Accounts.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  const raw_ptr<PrefService> pref_service_ = nullptr;

  // Cached fully-qualified email aliases service base URL.
  const GURL email_aliases_service_base_url_;

  // WeakPtrFactory to safely bind callbacks across async network operations.
  base::WeakPtrFactory<EmailAliasesService> weak_factory_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
