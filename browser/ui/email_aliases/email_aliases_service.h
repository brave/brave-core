/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
#define BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace email_aliases {

class EmailAliasesService : public KeyedService,
                            public mojom::EmailAliasesService {
 public:
  EmailAliasesService();
  ~EmailAliasesService() override;

  // KeyedService:
  void Shutdown() override;

  // mojom::EmailAliasesService:
  void RequestAuthentication(const std::string& auth_email,
                             RequestAuthenticationCallback callback) override;
  void CancelAuthenticationOrLogout() override;
  void GenerateAlias(GenerateAliasCallback callback) override;
  void UpdateAlias(const std::string& alias_email,
                   const std::optional<std::string>& note,
                   UpdateAliasCallback callback) override;
  void DeleteAlias(const std::string& alias_email,
                   DeleteAliasCallback callback) override;
  void AddObserver(mojo::PendingRemote<mojom::EmailAliasesServiceObserver>
                       observer) override;

  // Binds the mojom interface to this service
  void BindInterface(
      mojo::PendingReceiver<mojom::EmailAliasesService> receiver);

 private:
  mojo::ReceiverSet<mojom::EmailAliasesService> receivers_;
  mojo::RemoteSet<mojom::EmailAliasesServiceObserver> observers_;

  base::WeakPtrFactory<EmailAliasesService> weak_factory_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_H_
