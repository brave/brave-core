/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/get_service_token.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/mojom/get_service_token.mojom.h"
#include "brave/components/brave_account/state_internal.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::ServiceToken;
using internal::MakeClientError;
using internal::MakeRequest;
using internal::MakeServerError;

GetServiceToken::GetServiceToken(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor)
    : FlowBase(account_state_prefs, std::move(url_loader_factory), encryptor) {}

GetServiceToken::~GetServiceToken() = default;

void GetServiceToken::operator()(
    mojom::Service service,
    mojom::Authentication::GetServiceTokenCallback callback) {
  CHECK(service != mojom::Service::kAccounts);
  std::string service_name(kServiceToString.at(service));
  if (auto service_token =
          Decrypt(account_state_prefs_->GetCachedServiceToken(service_name));
      !service_token.empty()) {
    return std::move(callback).Run(
        mojom::GetServiceTokenResult::New(std::move(service_token)));
  }

  auto authentication_token =
      GetDecryptedAuthenticationToken<mojom::GetServiceTokenError>();
  if (!authentication_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(authentication_token).error()));
  }

  auto request = MakeRequest<WithHeaders<ServiceToken::Request>>();
  SetBearerToken(request, *authentication_token);
  request.body.service = service_name;

  SendStateOwnedRequest<ServiceToken>(
      std::move(request),
      base::BindOnce(&GetServiceToken::OnResponse, weak_factory_.GetWeakPtr(),
                     std::move(service_name), std::move(callback)));
}

void GetServiceToken::OnResponse(
    const std::string& service_name,
    mojom::Authentication::GetServiceTokenCallback callback,
    ServiceToken::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::GetServiceTokenError>(
            response.status_code.value_or(response.net_error),
            mojom::GetServiceTokenServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody              ]> ==>
          // expected<SuccessBody, [GetServiceTokenErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::GetServiceTokenError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody             ], GetServiceTokenErrorPtr> ==>
          // expected<[GetServiceTokenResultPtr], GetServiceTokenErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::GetServiceTokenResultPtr,
                                          mojom::GetServiceTokenErrorPtr> {
            if (success_body.auth_token.empty()) {
              return base::unexpected(
                  MakeServerError<mojom::GetServiceTokenError>(
                      status_code,
                      mojom::GetServiceTokenServerErrorCode::kInvalidResponse));
            }

            auto encrypted_service_token = Encrypt(success_body.auth_token);
            if (encrypted_service_token.empty()) {
              return base::unexpected(
                  MakeClientError<mojom::GetServiceTokenError>(
                      mojom::GetServiceTokenClientErrorCode::
                          kServiceTokenEncryptionFailed));
            }

            account_state_prefs_->CacheServiceToken(
                service_name, std::move(encrypted_service_token));

            return mojom::GetServiceTokenResult::New(
                std::move(success_body.auth_token));
          });

  std::move(callback).Run(std::move(result));
}

}  // namespace brave_account
