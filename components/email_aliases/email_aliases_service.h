/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class GURL;

using BodyAsStringCallback =
    base::OnceCallback<void(std::optional<std::string> response_body)>;

namespace email_aliases {

class EmailAliasesService : public KeyedService,
                            public mojom::EmailAliasesService {
 public:
  EmailAliasesService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~EmailAliasesService() override;

  // KeyedService:
  void Shutdown() override;

  // mojom::EmailAliasesService:
  void RequestAuthentication(const std::string& auth_email,
                             RequestAuthenticationCallback callback) override;
  void CancelAuthenticationOrLogout(
      CancelAuthenticationOrLogoutCallback callback) override;
  void GenerateAlias(GenerateAliasCallback callback) override;
  void UpdateAlias(const std::string& alias_email,
                   const std::optional<std::string>& note,
                   UpdateAliasCallback callback) override;
  void DeleteAlias(const std::string& alias_email,
                   DeleteAliasCallback callback) override;
  void AddObserver(mojo::PendingRemote<mojom::EmailAliasesServiceObserver>
                       observer) override;

  // Response handlers
  void OnRequestAuthenticationResponse(
      RequestAuthenticationCallback callback,
      std::optional<std::string> response_body);
  void OnRequestSessionResponse(std::optional<std::string> response_body);

  // Binds the mojom interface to this service
  void BindInterface(
      mojo::PendingReceiver<mojom::EmailAliasesService> receiver);

  std::string GetAuthTokenForTesting() const;

 private:
  void ApiFetch(const GURL& url,
                const char* method,
                const std::optional<std::string>& bearer_token,
                const base::Value::Dict& bodyValue,
                BodyAsStringCallback download_to_string_callback);
  void RequestSession();
  void NotifyObserversAuthStateChanged(mojom::AuthenticationStatus status);

  mojo::ReceiverSet<mojom::EmailAliasesService> receivers_;
  mojo::RemoteSet<mojom::EmailAliasesServiceObserver> observers_;
  std::string verification_token_;
  std::string auth_token_;
  std::string auth_email_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> verify_init_simple_url_loader_;
  std::unique_ptr<network::SimpleURLLoader> verify_result_simple_url_loader_;
  // Cached URLs computed once per service lifetime
  std::string verify_init_url_;
  std::string verify_result_url_;
  base::WeakPtrFactory<EmailAliasesService> weak_factory_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
