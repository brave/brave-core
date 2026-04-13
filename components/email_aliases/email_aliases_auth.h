/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_

#include "base/functional/callback_helpers.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefRegistrySimple;

namespace email_aliases {

class EmailAliasesAuth : public brave_account::mojom::AuthenticationObserver {
 public:
  using OnChangedCallback = base::RepeatingClosure;

  explicit EmailAliasesAuth(
      mojo::PendingRemote<brave_account::mojom::Authentication>
          brave_account_auth,
      OnChangedCallback on_changed = base::DoNothing());
  ~EmailAliasesAuth() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  bool IsAuthenticated() const;
  std::string GetAuthEmail() const;
  void GetServiceToken(
      brave_account::mojom::Authentication::GetServiceTokenCallback callback);

  void SetAuthEmailForTesting(const std::string& email);

 private:
  void OnDisconnect();

  // brave_account::mojom::AuthenticationObserver:
  void OnAccountStateChanged(
      brave_account::mojom::AccountStatePtr state) override;

  mojo::Remote<brave_account::mojom::Authentication> brave_account_auth_;

  OnChangedCallback on_changed_;

  brave_account::mojom::AccountStatePtr current_auth_state_;

  mojo::Receiver<brave_account::mojom::AuthenticationObserver> receiver_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_AUTH_H_
