/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_service.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/features.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

namespace {

const char* GetAccountsServiceBaseURL() {
#if defined(BRAVE_ACCOUNT_API_ENDPOINT)
  return BRAVE_ACCOUNT_API_ENDPOINT;
#else
  return "https://accounts.bsg.bravesoftware.com/v2";
#endif
}

const char kAccountsServiceRequestPath[] = "/verify/init";
const char kAccountsServiceVerifyPath[] = "/verify/result";

const char* GetBraveApiKey() {
#if defined(BRAVE_ACCOUNT_API_KEY)
  return BRAVE_ACCOUNT_API_KEY;
#else
  return "";
#endif
}

const net::NetworkTrafficAnnotationTag traffic_annotation =
    net::DefineNetworkTrafficAnnotation("email_aliases_mapping_service", R"(
      semantics {
        sender: "Email Aliases service"
        description:
          "Call Email Aliases Mapping Service API"
        trigger:
          "When the user connects to the Email Mapping Service, to "
          "Generate, Create, Read, Update, or Delete Email Aliases. "
        destination: BRAVE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: YES
    })");

constexpr int kMaxResponseLength = 32768;

}  // namespace

std::string GetAccountsServiceRequestURL() {
  return std::string(GetAccountsServiceBaseURL()) + kAccountsServiceRequestPath;
}

std::string GetAccountsServiceVerifyURL() {
  return std::string(GetAccountsServiceBaseURL()) + kAccountsServiceVerifyPath;
}

EmailAliasesService::EmailAliasesService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {
  CHECK(base::FeatureList::IsEnabled(email_aliases::kEmailAliases));
}

EmailAliasesService::~EmailAliasesService() = default;

void EmailAliasesService::Shutdown() {
  receivers_.Clear();
  observers_.Clear();
}

void EmailAliasesService::BindInterface(
    mojo::PendingReceiver<mojom::EmailAliasesService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void EmailAliasesService::NotifyObserversAuthStateChanged(
    mojom::AuthenticationStatus status) {
  for (auto& observer : observers_) {
    observer->OnAuthStateChanged(
        mojom::AuthState::New(status, auth_email_, std::nullopt));
  }
}

void EmailAliasesService::ApiFetch(
    const GURL& url,
    const char* method,
    const std::optional<std::string>& bearer_token,
    const base::Value::Dict& bodyValue,
    BodyAsStringCallback download_to_string_callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = method;
  if (bearer_token) {
    resource_request->headers.SetHeader(
        "Authorization", std::string("Bearer ") + bearer_token.value());
  }
  resource_request->headers.SetHeader("X-API-key", GetBraveApiKey());
  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  if (!bodyValue.empty() && method != net::HttpRequestHeaders::kGetMethod &&
      method != net::HttpRequestHeaders::kHeadMethod) {
    auto body = base::WriteJson(bodyValue);
    CHECK(body);
    simple_url_loader_->AttachStringForUpload(body.value(), "application/json");
  }
  simple_url_loader_->DownloadToString(url_loader_factory_.get(),
                                       std::move(download_to_string_callback),
                                       kMaxResponseLength);
}

void EmailAliasesService::RequestAuthentication(
    const std::string& auth_email,
    RequestAuthenticationCallback callback) {
  auth_email_ = auth_email;
  if (auth_email.empty()) {
    std::move(callback).Run(
        l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_EMAIL_PROVIDED));
    return;
  }
  const auto body_value = base::Value::Dict()
                              .Set("email", auth_email)
                              .Set("intent", "auth_token")
                              .Set("service", "email-aliases");
  ApiFetch(GURL(GetAccountsServiceRequestURL()),
           net::HttpRequestHeaders::kPostMethod, std::nullopt, body_value,
           base::BindOnce(&EmailAliasesService::OnRequestAuthenticationResponse,
                          weak_factory_.GetWeakPtr(), std::move(callback)));
}

void EmailAliasesService::OnRequestAuthenticationResponse(
    RequestAuthenticationCallback callback,
    std::optional<std::string> response_body) {
  if (!response_body) {
    std::move(callback).Run(
        l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_RESPONSE_BODY));
    return;
  }
  const auto response_body_dict = base::JSONReader::Read(*response_body);
  if (!response_body_dict || !response_body_dict->is_dict()) {
    std::move(callback).Run(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_ERROR_INVALID_RESPONSE_BODY));
    return;
  }
  const auto* verification_token_ptr =
      response_body_dict->GetDict().FindString("verificationToken");
  if (!verification_token_ptr) {
    std::move(callback).Run(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_ERROR_NO_VERIFICATION_TOKEN));
    return;
  }
  // Success
  verification_token_ = *verification_token_ptr;
  NotifyObserversAuthStateChanged(mojom::AuthenticationStatus::kAuthenticating);
  std::move(callback).Run(std::nullopt);
  RequestSession();
}

void EmailAliasesService::RequestSession() {
  auto body_value = base::Value::Dict().Set("wait", true);
  ApiFetch(GURL(GetAccountsServiceVerifyURL()),
           net::HttpRequestHeaders::kPostMethod, verification_token_,
           body_value,
           base::BindOnce(&EmailAliasesService::OnRequestSessionResponse,
                          weak_factory_.GetWeakPtr()));
}

void EmailAliasesService::OnRequestSessionResponse(
    std::optional<std::string> response_body) {
  if (!response_body) {
    return;
  }
  const auto response_body_dict = base::JSONReader::Read(*response_body);
  if (!response_body_dict || !response_body_dict->is_dict()) {
    return;
  }
  const auto* auth_token_ptr =
      response_body_dict->GetDict().FindString("authToken");
  if (!auth_token_ptr) {
    RequestSession();
    return;
  }
  auth_token_ = *auth_token_ptr;
  NotifyObserversAuthStateChanged(mojom::AuthenticationStatus::kAuthenticated);
}

void EmailAliasesService::CancelAuthenticationOrLogout(
    CancelAuthenticationOrLogoutCallback callback) {
  verification_token_.clear();
  auth_token_.clear();
  NotifyObserversAuthStateChanged(
      mojom::AuthenticationStatus::kUnauthenticated);
  std::move(callback).Run();
}

void EmailAliasesService::GenerateAlias(GenerateAliasCallback callback) {
  mojom::GenerateAliasResultPtr result =
      mojom::GenerateAliasResult::NewErrorMessage("Not implemented");
  std::move(callback).Run(std::move(result));
}

void EmailAliasesService::UpdateAlias(const std::string& alias_email,
                                      const std::optional<std::string>& note,
                                      UpdateAliasCallback callback) {
  // TODO: Implement alias update logic
  std::move(callback).Run("Not implemented");
}

void EmailAliasesService::DeleteAlias(const std::string& alias_email,
                                      DeleteAliasCallback callback) {
  // TODO: Implement alias deletion logic
  std::move(callback).Run("Not implemented");
}

void EmailAliasesService::AddObserver(
    mojo::PendingRemote<mojom::EmailAliasesServiceObserver> observer) {
  auto id = observers_.Add(std::move(observer));
  auto* remote = observers_.Get(id);
  if (remote) {
    remote->OnAuthStateChanged(mojom::AuthState::New(
        mojom::AuthenticationStatus::kUnauthenticated, "", std::nullopt));
  }
}

std::string EmailAliasesService::GetAuthTokenForTesting() const {
  return auth_token_;
}

}  // namespace email_aliases
