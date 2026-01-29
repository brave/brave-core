/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_service.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_api.h"
#include "brave/components/email_aliases/features.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

namespace {

const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_email_aliases_service", R"(
      semantics {
        sender: "Email Aliases Service"
        description:
          "Call to Email Aliases Service API"
        trigger:
          "When the user communicates with Email Aliases"
        destination: BRAVE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: YES
    })");

// Handles response from specified endpoint (which must expose a `message`
// field). On success, returns base::ok(Response) when request is
// successfull and response's message == |expected_message|. On failure, returns
// base::unexpected with a user-facing error string.
template <typename Response>
base::expected<typename Response::SuccessBody, std::string>
HandleEmailAliasesResponse(Response response,
                           std::string_view expected_message = {}) {
  if (!response.body) {
    return base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY));
  }
  return std::move(*response.body)
      .transform_error([&](auto error) {
        return l10n_util::GetStringFUTF8(
            IDS_EMAIL_ALIASES_SERVICE_REPORTED_ERROR,
            base::UTF8ToUTF16(error.message));
      })
      .and_then(
          [&](auto response)
              -> base::expected<typename Response::SuccessBody, std::string> {
            if (!expected_message.empty() &&
                response.message != expected_message) {
              return base::unexpected(l10n_util::GetStringUTF8(
                  IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY));
            }
            return base::ok(std::move(response));
          });
}

template <typename Request>
auto MakeRequest(const std::string& bearer_token) {
  Request request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  brave_account::endpoint_client::SetBearerToken(request, bearer_token);
  request.headers.SetHeader("Brave-Key", BUILDFLAG(BRAVE_SERVICES_KEY));
  request.headers.SetHeader("X-API-key", BUILDFLAG(BRAVE_SERVICES_KEY));
  return request;
}

}  // namespace

EmailAliasesService::EmailAliasesService(
    brave_account::mojom::Authentication* brave_account_auth,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* pref_service)
    : brave_account_auth_(brave_account_auth),
      url_loader_factory_(url_loader_factory),
      pref_service_(pref_service) {
  CHECK(base::FeatureList::IsEnabled(email_aliases::features::kEmailAliases));
  CHECK(brave_account_auth_);
  CHECK(pref_service_);

  auth_.emplace(pref_service_, brave_account_auth_,
                base::BindRepeating(&EmailAliasesService::OnAuthChanged,
                                    weak_factory_.GetWeakPtr()));
}

EmailAliasesService::~EmailAliasesService() = default;

// static
void EmailAliasesService::RegisterProfilePrefs(PrefRegistrySimple* registry) {}

void EmailAliasesService::Shutdown() {
  receivers_.Clear();
  observers_.Clear();
}

void EmailAliasesService::BindInterface(
    mojo::PendingReceiver<mojom::EmailAliasesService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

EmailAliasesAuth* EmailAliasesService::GetAuth() {
  return &auth_.value();
}

std::string EmailAliasesService::GetAuthEmail() const {
  return auth_->GetAuthEmail();
}

mojom::AuthenticationStatus EmailAliasesService::GetCurrentStatus() {
  if (auth_->IsAuthenticated()) {
    return mojom::AuthenticationStatus::kAuthenticated;
  }
  return mojom::AuthenticationStatus::kUnauthenticated;
}

void EmailAliasesService::OnAuthChanged() {
  const auto status = GetCurrentStatus();
  const auto email = GetAuthEmail();
  for (auto& observer : observers_) {
    observer->OnAuthStateChanged(mojom::AuthState::New(status, email));
  }
}

void EmailAliasesService::GenerateAlias(GenerateAliasCallback callback) {
  auto wrapper = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), base::unexpected(std::string()));
  auth_->GetServiceToken(
      base::BindOnce(&EmailAliasesService::GenerateAliasWithToken,
                     weak_factory_.GetWeakPtr(), std::move(wrapper)));
}

void EmailAliasesService::UpdateAlias(const std::string& alias_email,
                                      const std::optional<std::string>& note,
                                      UpdateAliasCallback callback) {
  auto wrapper = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), base::unexpected(std::string()));
  auth_->GetServiceToken(base::BindOnce(
      &EmailAliasesService::UpdateAliasWithToken, weak_factory_.GetWeakPtr(),
      alias_email, note, std::move(wrapper)));
}

void EmailAliasesService::DeleteAlias(const std::string& alias_email,
                                      DeleteAliasCallback callback) {
  auto wrapper = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), base::unexpected(std::string()));
  auth_->GetServiceToken(base::BindOnce(
      &EmailAliasesService::DeleteAliasWithToken, weak_factory_.GetWeakPtr(),
      alias_email, std::move(wrapper)));
}

void EmailAliasesService::AddObserver(
    mojo::PendingRemote<mojom::EmailAliasesServiceObserver> observer) {
  auto id = observers_.Add(std::move(observer));
  auto* remote = observers_.Get(id);
  if (remote) {
    remote->OnAuthStateChanged(
        mojom::AuthState::New(GetCurrentStatus(), GetAuthEmail()));
  }
}

bool EmailAliasesService::IsAuthenticated() const {
  return auth_->IsAuthenticated();
}

void EmailAliasesService::OnGenerateAliasResponse(
    GenerateAliasCallback user_callback,
    endpoints::GenerateAlias::Response response) {
  auto parsed = HandleEmailAliasesResponse(std::move(response), "created");
  auto result =
      parsed.has_value()
          ? base::expected<std::string, std::string>(parsed.value().alias)
          : base::unexpected(parsed.error());
  std::move(user_callback).Run(std::move(result));
}

void EmailAliasesService::OnEditAliasResponse(
    base::OnceCallback<void(base::expected<std::monostate, std::string>)>
        user_callback,
    bool update_expected,
    endpoints::UpdateAlias::Response response) {
  auth_->GetServiceToken(
      base::BindOnce(&EmailAliasesService::RefreshAliasesWithToken,
                     weak_factory_.GetWeakPtr()));

  auto parsed = HandleEmailAliasesResponse(
      std::move(response), update_expected ? "updated" : "deleted");
  auto result =
      parsed.has_value()
          ? base::expected<std::monostate, std::string>(std::monostate{})
          : base::unexpected(parsed.error());
  std::move(user_callback).Run(std::move(result));
}

void EmailAliasesService::RefreshAliasesWithToken(TokenResult token) {
  if (token.has_value()) {
    auto request = MakeRequest<brave_account::endpoint_client::WithHeaders<
        endpoints::AliasList::Request>>(token.value()->serviceToken);
    brave_account::endpoint_client::Client<endpoints::AliasList>::Send(
        url_loader_factory_, std::move(request),
        base::BindOnce(&EmailAliasesService::OnRefreshAliasesResponse,
                       weak_factory_.GetWeakPtr()));
  }
}

void EmailAliasesService::GenerateAliasWithToken(GenerateAliasCallback callback,
                                                 TokenResult token) {
  if (token.has_value()) {
    auto request = MakeRequest<brave_account::endpoint_client::WithHeaders<
        endpoints::GenerateAlias::Request>>(token.value()->serviceToken);
    brave_account::endpoint_client::Client<endpoints::GenerateAlias>::Send(
        url_loader_factory_, std::move(request),
        base::BindOnce(&EmailAliasesService::OnGenerateAliasResponse,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    std::move(callback).Run(base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY)));
  }
}

void EmailAliasesService::UpdateAliasWithToken(
    const std::string& alias_email,
    const std::optional<std::string>& note,
    UpdateAliasCallback callback,
    TokenResult token) {
  if (token.has_value()) {
    // Build JSON using IDL-defined shape
    auto request = MakeRequest<brave_account::endpoint_client::WithHeaders<
        endpoints::UpdateAlias::Request>>(token.value()->serviceToken);
    request.alias = alias_email;
    request.status = "active";  // For now, we only support active aliases.

    // TODO(https://github.com/brave/brave-browser/issues/49229):
    // Add support for storing alias note in the client.

    brave_account::endpoint_client::Client<endpoints::UpdateAlias>::Send(
        url_loader_factory_, std::move(request),
        base::BindOnce(&EmailAliasesService::OnEditAliasResponse,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       /*update_expected=*/true));
  } else {
    std::move(callback).Run(base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY)));
  }
}

void EmailAliasesService::DeleteAliasWithToken(const std::string& alias_email,
                                               DeleteAliasCallback callback,
                                               TokenResult token) {
  if (token.has_value()) {
    auto request = MakeRequest<brave_account::endpoint_client::WithHeaders<
        endpoints::DeleteAlias::Request>>(token.value()->serviceToken);
    request.alias = alias_email;
    brave_account::endpoint_client::Client<endpoints::DeleteAlias>::Send(
        url_loader_factory_, std::move(request),
        base::BindOnce(&EmailAliasesService::OnEditAliasResponse,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       /*update_expected=*/false));
  } else {
    // Auth
  }
}

void EmailAliasesService::OnRefreshAliasesResponse(
    endpoints::AliasList::Response response) {
  // TODO(https://github.com/brave/brave-browser/issues/48959):
  // In this function, when an error happens, we should show
  // an error message to the user (requires design work)
  if (!response.body) {
    LOG(ERROR) << "Email Aliases service error: No response body";
    return;
  }
  if (!response.body->has_value()) {
    LOG(ERROR) << "Email Aliases service error: Invalid response format";
    return;
  }
  std::vector<email_aliases::mojom::AliasPtr> aliases;
  for (const auto& entry : response.body.value()->result) {
    auto alias_obj = email_aliases::mojom::Alias::New();
    alias_obj->email = entry.alias;
    aliases.push_back(std::move(alias_obj));
  }
  for (auto& observer : observers_) {
    observer->OnAliasesUpdated(mojo::Clone(aliases));
  }
}

}  // namespace email_aliases
