/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/delete_account.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/accounts_delete.h"
#include "brave/components/brave_account/flows/make_request.h"
#include "brave/components/brave_account/mojom/delete_account.mojom.h"
#include "brave/components/brave_account/state_internal.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::AccountsDelete;
using internal::MakeRequest;
using internal::MakeServerError;

DeleteAccount::DeleteAccount(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor)
    : FlowBase(account_state_prefs, std::move(url_loader_factory), encryptor) {}

DeleteAccount::~DeleteAccount() = default;

void DeleteAccount::operator()(
    mojom::Authentication::DeleteAccountCallback callback) {
  auto authentication_token =
      GetDecryptedAuthenticationToken<mojom::DeleteAccountError>();
  if (!authentication_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(authentication_token).error()));
  }

  auto request = MakeRequest<WithHeaders<AccountsDelete::Request>>();
  SetBearerToken(request, *authentication_token);

  SendFlowOwnedRequest<AccountsDelete>(
      std::move(request),
      base::BindOnce(&DeleteAccount::OnResponse, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void DeleteAccount::OnResponse(
    mojom::Authentication::DeleteAccountCallback callback,
    AccountsDelete::Response response) {
  if (response.status_code != net::HTTP_NO_CONTENT) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::DeleteAccountError>(
            response.status_code.value_or(response.net_error),
            mojom::DeleteAccountServerErrorCode::kNull)));
  }

  // See `FlowBase`'s class comment on ordering.
  std::move(callback).Run(mojom::DeleteAccountResult::New());

  // LoggedIn ==> LoggedOut (state swap).
  account_state_prefs_->SetLoggedOut();
}

}  // namespace brave_account
