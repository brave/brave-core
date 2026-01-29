/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_auth.h"
#include "brave/components/email_aliases/email_aliases_endpoints.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace network {
class SharedURLLoaderFactory;
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
      brave_account::mojom::Authentication* brave_account_auth,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* pref_service);
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

  EmailAliasesAuth* GetAuth();

 private:
  // Callback that receives the response body as an optional string.
  using TokenResult =
      base::expected<brave_account::mojom::GetServiceTokenResultPtr,
                     brave_account::mojom::GetServiceTokenErrorPtr>;

  std::string GetAuthEmail() const;

  mojom::AuthenticationStatus GetCurrentStatus();

  void OnAuthChanged();

  // Refreshes the aliases list from the server and notifies observers.
  void RefreshAliasesWithToken(TokenResult token);

  void GenerateAliasWithToken(GenerateAliasCallback user_callback,
                              TokenResult token);
  void UpdateAliasWithToken(const std::string& alias_email,
                            const std::optional<std::string>& note,
                            UpdateAliasCallback callback,
                            TokenResult token);
  void DeleteAliasWithToken(const std::string& alias_email,
                            DeleteAliasCallback callback,
                            TokenResult token);

  // Parses and applies the aliases list received from the backend.
  void OnRefreshAliasesResponse(endpoints::AliasList::Response response);

  // Processes the server response for a generate-alias request.
  void OnGenerateAliasResponse(GenerateAliasCallback user_callback,
                               endpoints::GenerateAlias::Response response);

  // Common handler for alias edit responses (update/delete).
  void OnEditAliasResponse(
      base::OnceCallback<void(base::expected<std::monostate, std::string>)>
          user_callback,
      bool update_expected,
      endpoints::UpdateAlias::Response response);

  // Bound Mojo receivers for the EmailAliasesService interface.
  mojo::ReceiverSet<mojom::EmailAliasesService> receivers_;

  // Connected observers that receive authentication state updates.
  mojo::RemoteSet<mojom::EmailAliasesServiceObserver> observers_;

  std::optional<EmailAliasesAuth> auth_;

  const raw_ptr<brave_account::mojom::Authentication> brave_account_auth_ =
      nullptr;

  // URL loader factory used to issue network requests to Brave Accounts.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  const raw_ptr<PrefService> pref_service_ = nullptr;

  // WeakPtrFactory to safely bind callbacks across async network operations.
  base::WeakPtrFactory<EmailAliasesService> weak_factory_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
