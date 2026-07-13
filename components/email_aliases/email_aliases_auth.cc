/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_auth.h"

namespace email_aliases {

EmailAliasesAuth::EmailAliasesAuth(
    mojo::PendingRemote<brave_account::mojom::Authentication>
        brave_account_auth,
    OnChangedCallback on_changed)
    : brave_account_auth_(std::move(brave_account_auth)),
      on_changed_(std::move(on_changed)) {
  CHECK(brave_account_auth_);
  CHECK(on_changed_);

  brave_account_auth_.set_disconnect_handler(base::BindOnce(
      &EmailAliasesAuth::OnDisconnect,
      base::Unretained(
          this)));  // Unretained is safe because we own the remote<>

  brave_account_auth_->AddObserver(receiver_.BindNewPipeAndPassRemote());
}

EmailAliasesAuth::~EmailAliasesAuth() = default;

bool EmailAliasesAuth::IsAuthenticated() const {
  return !auth_email_.empty();
}

std::string EmailAliasesAuth::GetAuthEmail() const {
  return auth_email_;
}

void EmailAliasesAuth::GetServiceToken(
    brave_account::mojom::Authentication::GetServiceTokenCallback callback) {
  if (brave_account_auth_) {
    brave_account_auth_->GetServiceToken(
        brave_account::mojom::Service::kEmailAliases, std::move(callback));
  } else {
    // TODO(https://github.com/brave/brave-browser/issues/54976)
    std::move(callback).Run(base::unexpected(
        brave_account::mojom::GetServiceTokenError::NewClientError(
            brave_account::mojom::GetServiceTokenClientError::New(
                brave_account::mojom::GetServiceTokenClientErrorCode::
                    kUnexpected))));
  }
}

void EmailAliasesAuth::OnDisconnect() {
  brave_account_auth_.reset();
  auth_email_.clear();
  on_changed_.Run();
}

void EmailAliasesAuth::OnAccountStateChanged(
    brave_account::mojom::AccountStatePtr state) {
  if (state->is_logged_in()) {
    auth_email_ = state->get_logged_in()->email;
  } else {
    auth_email_.clear();
  }
  on_changed_.Run();
}

}  // namespace email_aliases
