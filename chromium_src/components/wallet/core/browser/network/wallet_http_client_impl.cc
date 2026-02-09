/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/wallet/core/browser/network/wallet_http_client_impl.h"

#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace wallet {

WalletHttpClientImpl::WalletHttpClientImpl(
    signin::IdentityManager* identity_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {}

WalletHttpClientImpl::~WalletHttpClientImpl() = default;

void WalletHttpClientImpl::UpsertPublicPass(Pass pass,
                                            UpsertPublicPassCallback callback) {
  std::move(callback).Run(
      base::unexpected(WalletHttpClient::WalletRequestError::kGenericError));
}

void WalletHttpClientImpl::UpsertPrivatePass(
    PrivatePass pass,
    UpsertPrivatePassCallback callback) {
  std::move(callback).Run(
      base::unexpected(WalletHttpClient::WalletRequestError::kGenericError));
}

void WalletHttpClientImpl::GetUnmaskedPass(std::string_view pass_id,
                                           GetUnmaskedPassCallback callback) {
  std::move(callback).Run(
      base::unexpected(WalletHttpClient::WalletRequestError::kGenericError));
}

}  // namespace wallet
