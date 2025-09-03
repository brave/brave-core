/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_service.h"

#include <memory>
#include <utility>
#include <variant>

#include "absl/strings/str_format.h"
#include "base/check.h"
#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/types/expected.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_api.h"
#include "brave/components/email_aliases/features.h"
#include "brave/components/email_aliases/email_aliases_api_key.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/clone_traits.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

namespace {

constexpr char kAccountServiceEndpoint[] = "https://%s/v2/%s";
constexpr char kAccountsServiceVerifyInitPath[] = "verify/init";
constexpr char kAccountsServiceVerifyResultPath[] = "verify/result";

constexpr char kEmailAliasesServiceBaseURL[] = "https://aliases.bravesoftware.com";
constexpr char kEmailAliasesServiceManagePath[] = "/manage";

// Minimum interval between verify/result polls
constexpr base::TimeDelta kSessionPollInterval = base::Seconds(2);
// Maximum total polling duration for a single verification flow.
constexpr base::TimeDelta kMaxSessionPollDuration = base::Minutes(30);

const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_accounts_service", R"(
      semantics {
        sender: "Email Aliases service"
        description:
          "Call Brave Accounts Service API"
        trigger:
          "When the user requests to authenticate with Email Aliases"
        destination: BRAVE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: YES
    })");

constexpr int kMaxResponseLength = 32768;

}  // namespace

// static
GURL EmailAliasesService::GetAccountsServiceVerifyInitURL() {
  return GURL(absl::StrFormat(kAccountServiceEndpoint,
                              brave_domains::GetServicesDomain("accounts.bsg"),
                              kAccountsServiceVerifyInitPath));
}

GURL EmailAliasesService::GetAccountsServiceVerifyResultURL() {
  return GURL(absl::StrFormat(kAccountServiceEndpoint,
                              brave_domains::GetServicesDomain("accounts.bsg"),
                              kAccountsServiceVerifyResultPath));
}

// static
GURL EmailAliasesService::GetEmailAliasesServiceBaseURL() {
  return GURL(absl::StrFormat("%s%s",
    kEmailAliasesServiceBaseURL, kEmailAliasesServiceManagePath));
}

// static
std::string EmailAliasesService::GetEmailAliasesServiceAPIKey() {
  return BUILDFLAG(EMAIL_ALIASES_API_KEY);
}

EmailAliasesService::EmailAliasesService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory),
      verify_init_url_(GetAccountsServiceVerifyInitURL()),
      verify_result_url_(GetAccountsServiceVerifyResultURL()),
      email_aliases_service_base_url_(GetEmailAliasesServiceBaseURL()),
      email_aliases_api_key_(GetEmailAliasesServiceAPIKey()) {
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
    mojom::AuthenticationStatus status,
    const std::optional<std::string>& error_message) {
  for (auto& observer : observers_) {
    observer->OnAuthStateChanged(
        mojom::AuthState::New(status, auth_email_, error_message));
  }
}

void EmailAliasesService::ResetVerificationFlow() {
  verification_simple_url_loader_.reset();
  session_request_timer_.Stop();
  verification_token_.clear();
  auth_token_.clear();
}

void EmailAliasesService::RequestAuthentication(
    const std::string& auth_email,
    RequestAuthenticationCallback callback) {
  ResetVerificationFlow();
  auth_email_ = auth_email;
  if (auth_email.empty()) {
    std::move(callback).Run(base::unexpected(
        l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_EMAIL_PROVIDED)));
    return;
  }
  AuthenticationRequest auth_request;
  auth_request.email = auth_email;
  auth_request.intent = "auth_token";
  auth_request.service = "email-aliases";
  std::optional<std::string> body = base::WriteJson(auth_request.ToValue());
  CHECK(body);
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = verify_init_url_;
  resource_request->method = net::HttpRequestHeaders::kPostMethod;
  verification_simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kTrafficAnnotation);
  verification_simple_url_loader_->SetRetryOptions(
      /* max_retries=*/3,
      network::SimpleURLLoader::RETRY_ON_5XX |
          network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);
  verification_simple_url_loader_->AttachStringForUpload(*body,
                                                         "application/json");
  verification_simple_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&EmailAliasesService::OnRequestAuthenticationResponse,
                     base::Unretained(this), std::move(callback)),
      kMaxResponseLength);
}

void EmailAliasesService::OnRequestAuthenticationResponse(
    RequestAuthenticationCallback callback,
    std::optional<std::string> response_body) {
  verification_simple_url_loader_.reset();
  if (!response_body) {
    std::move(callback).Run(base::unexpected(
        l10n_util::GetStringUTF8(IDS_EMAIL_ALIASES_ERROR_NO_RESPONSE_BODY)));
    return;
  }
  const auto response_body_dict = base::JSONReader::ReadDict(*response_body);
  if (!response_body_dict) {
    std::move(callback).Run(base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_ERROR_INVALID_RESPONSE_BODY)));
    return;
  }
  auto error_message = ErrorResponse::FromValue(*response_body_dict);
  if (error_message) {
    LOG(ERROR) << "Email Aliases verification error: " << error_message->error;
    std::move(callback).Run(base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_ERROR_NO_VERIFICATION_TOKEN)));
    return;
  }
  auto parsed_auth = AuthenticationResponse::FromValue(*response_body_dict);
  if (!parsed_auth || parsed_auth->verification_token.empty()) {
    LOG(ERROR) << "Email Aliases verification error: No verification token";
    std::move(callback).Run(base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_ERROR_NO_VERIFICATION_TOKEN)));
    return;
  }
  // Success; set the verification token and notify observers.
  verification_token_ = parsed_auth->verification_token;
  NotifyObserversAuthStateChanged(mojom::AuthenticationStatus::kAuthenticating);
  std::move(callback).Run(base::ok(std::monostate{}));
  // Begin the polling window.
  RequestSession();
}

void EmailAliasesService::RequestSession() {
  CHECK(!verification_simple_url_loader_.get());
  if (verification_token_.empty()) {
    // No verification token; polling has been cancelled.
    return;
  }
  SessionRequest session_request;
  session_request.wait = true;
  std::optional<std::string> body = base::WriteJson(session_request.ToValue());
  CHECK(body);
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = verify_result_url_;
  resource_request->method = net::HttpRequestHeaders::kPostMethod;
  resource_request->headers.SetHeader(
      "Authorization", std::string("Bearer ") + verification_token_);
  verification_simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kTrafficAnnotation);
  verification_simple_url_loader_->AttachStringForUpload(*body,
                                                         "application/json");
  verification_simple_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&EmailAliasesService::OnRequestSessionResponse,
                     base::Unretained(this)),
      kMaxResponseLength);
}

void EmailAliasesService::OnRequestSessionResponse(
    std::optional<std::string> response_body) {
  verification_simple_url_loader_.reset();
  if (!response_body) {
    // No response body, log it and re-request.
    LOG(ERROR) << "Email Aliases service error: No response body";
    MaybeRequestSessionAgain();
    return;
  }
  const auto response_body_dict = base::JSONReader::ReadDict(*response_body);
  if (!response_body_dict) {
    // Invalid response body, log it and re-request.
    LOG(ERROR) << "Email Aliases service error: Invalid response body";
    MaybeRequestSessionAgain();
    return;
  }
  auto error_message = ErrorResponse::FromValue(*response_body_dict);
  if (error_message) {
    // An error has been reported by the server, indicating that verification
    // failed. Log it and notify observers.
    LOG(ERROR) << "Email Aliases service error: " << error_message->error;
    session_poll_elapsed_timer_.reset();
    NotifyObserversAuthStateChanged(
        mojom::AuthenticationStatus::kUnauthenticated,
        /*error_message=*/l10n_util::GetStringUTF8(
            IDS_EMAIL_ALIASES_ERROR_VERIFICATION_FAILED));
    return;
  }
  auto parsed_session = SessionResponse::FromValue(*response_body_dict);
  if (!parsed_session) {
    // No error message but unparseable response, log it and re-request.
    LOG(ERROR) << "Email Aliases service verification error: Parse error but "
                  "no error message";
    MaybeRequestSessionAgain();
    return;
  }
  if (!parsed_session->verified || !parsed_session->auth_token) {
    // Verification still in progress; no auth token yet. Re-request.
    MaybeRequestSessionAgain();
    return;
  }
  // Success; set the auth token and notify observers.
  auth_token_ = *parsed_session->auth_token;
  session_poll_elapsed_timer_.reset();
  NotifyObserversAuthStateChanged(mojom::AuthenticationStatus::kAuthenticated);
  // Kick off an initial aliases refresh on successful authentication.
  RefreshAliases();
}

void EmailAliasesService::MaybeRequestSessionAgain() {
  if (!session_poll_elapsed_timer_.has_value()) {
    session_poll_elapsed_timer_.emplace();
  }
  if (session_poll_elapsed_timer_->Elapsed() > kMaxSessionPollDuration) {
    LOG(ERROR) << "Email Aliases service verification error: exceeded max "
                  "poll duration";
    session_poll_elapsed_timer_.reset();
    NotifyObserversAuthStateChanged(
        mojom::AuthenticationStatus::kUnauthenticated,
        /*error_message=*/l10n_util::GetStringUTF8(
            IDS_EMAIL_ALIASES_ERROR_VERIFICATION_FAILED));
    return;
  }
  // Session request timer should not be running at this point.
  CHECK(!session_request_timer_.IsRunning());
  // Schedule the next request after a short interval.
  session_request_timer_.Start(
      FROM_HERE, kSessionPollInterval,
      base::BindOnce(&EmailAliasesService::RequestSession,
                     base::Unretained(this)));
}

void EmailAliasesService::CancelAuthenticationOrLogout(
    CancelAuthenticationOrLogoutCallback callback) {
  ResetVerificationFlow();
  std::move(callback).Run();
  NotifyObserversAuthStateChanged(
      mojom::AuthenticationStatus::kUnauthenticated);
}

void EmailAliasesService::GenerateAlias(GenerateAliasCallback callback) {
  base::Value::Dict body_value;  // empty JSON object
  ApiFetch(email_aliases_service_base_url_, net::HttpRequestHeaders::kPostMethod, body_value,
           base::BindOnce(&EmailAliasesService::OnGenerateAliasResponse,
                          weak_factory_.GetWeakPtr(), std::move(callback)));
}

void EmailAliasesService::UpdateAlias(const std::string& alias_email,
                                      const std::optional<std::string>& note,
                                      UpdateAliasCallback callback) {
  base::Value::Dict body_value;
  body_value.Set("alias", alias_email);
  body_value.Set("status", "active");
  if (note) {
    body_value.Set("note", *note);
  }
  ApiFetch(email_aliases_service_base_url_, net::HttpRequestHeaders::kPutMethod, body_value,
           base::BindOnce(&EmailAliasesService::OnUpdateAliasResponse,
                          weak_factory_.GetWeakPtr(), std::move(callback)));
}

void EmailAliasesService::DeleteAlias(const std::string& alias_email,
                                      DeleteAliasCallback callback) {
  base::Value::Dict body_value;
  body_value.Set("alias", alias_email);
  ApiFetch(email_aliases_service_base_url_, net::HttpRequestHeaders::kDeleteMethod, body_value,
           base::BindOnce(&EmailAliasesService::OnDeleteAliasResponse,
                          weak_factory_.GetWeakPtr(), std::move(callback)));
}

void EmailAliasesService::AddObserver(
    mojo::PendingRemote<mojom::EmailAliasesServiceObserver> observer) {
  auto id = observers_.Add(std::move(observer));
  auto* remote = observers_.Get(id);
  if (remote) {
    remote->OnAuthStateChanged(
        mojom::AuthState::New(mojom::AuthenticationStatus::kUnauthenticated,
                              /*email=*/"", /*error_message=*/std::nullopt));
  }
}

std::string EmailAliasesService::GetAuthTokenForTesting() const {
  return auth_token_;
}

void EmailAliasesService::ApiFetch(
    const GURL& url,
    const char* method,
    const base::Value::Dict& body_value,
    BodyAsStringCallback download_to_string_callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = method;
  resource_request->headers.SetHeader("Authorization",
                                      std::string("Bearer ") + auth_token_);
  resource_request->headers.SetHeader("X-API-key", email_aliases_api_key_);
  auto simple_url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), kTrafficAnnotation);
  // For non-GET/HEAD methods attach a JSON body. Backend expects text/plain.
  if (method != net::HttpRequestHeaders::kGetMethod &&
      method != net::HttpRequestHeaders::kHeadMethod) {
    auto body = base::WriteJson(body_value);
    CHECK(body);
    simple_url_loader->AttachStringForUpload(body.value(), "text/plain");
  }
  // Keep loader alive until completion by transferring ownership through the
  // callback. This allows multiple alias requests to run concurrently.
  auto* loader_ptr = simple_url_loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&EmailAliasesService::OnApiFetchDownloadToStringComplete,
                     weak_factory_.GetWeakPtr(),
                     std::move(download_to_string_callback),
                     std::move(simple_url_loader)),
      kMaxResponseLength);
}

void EmailAliasesService::OnApiFetchDownloadToStringComplete(
    BodyAsStringCallback callback,
    std::unique_ptr<network::SimpleURLLoader> /*owned_loader*/,
    std::unique_ptr<std::string> response_body) {
  std::optional<std::string> body_opt;
  if (response_body) {
    body_opt = std::move(*response_body);
  }
  std::move(callback).Run(std::move(body_opt));
}

void EmailAliasesService::OnGenerateAliasResponse(
    GenerateAliasCallback user_callback,
    std::optional<std::string> response_body) {
  std::string error_message;
  if (!response_body) {
    error_message = "No response from server.";
  } else {
    auto parsed = base::JSONReader::Read(*response_body);
    if (!parsed || !parsed->is_dict()) {
      error_message = "Invalid response format.";
    } else {
      const auto& dict = parsed->GetDict();
      const std::string* message = dict.FindString("message");
      const std::string* alias = dict.FindString("alias");
      if (message && *message == "created" && alias && !alias->empty()) {
        std::move(user_callback)
            .Run(base::ok(*alias));
        RefreshAliases();
        return;
      } else if (message && *message != "created") {
        error_message = *message;
      } else {
        error_message = "Alias not available.";
      }
    }
  }
  std::move(user_callback)
      .Run(base::unexpected(error_message));
}

void EmailAliasesService::OnUpdateAliasResponse(
    UpdateAliasCallback user_callback,
    std::optional<std::string> /*response_body*/) {
  // TODO: handle response body -- errors?
  std::move(user_callback).Run(base::ok(std::monostate{}));
  RefreshAliases();
}

void EmailAliasesService::OnDeleteAliasResponse(
    DeleteAliasCallback user_callback,
    std::optional<std::string> /*response_body*/) {
  // TODO: handle response body -- errors?
  std::move(user_callback).Run(base::ok(std::monostate{}));
  RefreshAliases();
}

void EmailAliasesService::RefreshAliases() {
  ApiFetch(email_aliases_service_base_url_.Resolve("?status=active"), net::HttpRequestHeaders::kGetMethod, base::Value::Dict(),
           base::BindOnce(&EmailAliasesService::OnRefreshAliasesResponse,
                          weak_factory_.GetWeakPtr()));
}

void EmailAliasesService::OnRefreshAliasesResponse(
    std::optional<std::string> response_body) {
  if (!response_body) {
    return;
  }
  auto parsed = base::JSONReader::Read(*response_body);
  if (!parsed || !parsed->is_list()) {
    return;
  }
  std::vector<email_aliases::mojom::AliasPtr> aliases;
  for (const auto& alias_val : parsed->GetList()) {
    if (!alias_val.is_dict()) {
      continue;
    }
    const std::string* email = alias_val.GetDict().FindString("email");
    const std::string* alias = alias_val.GetDict().FindString("alias");
    if (!email || !alias) {
      continue;
    }
    auto alias_obj = email_aliases::mojom::Alias::New();
    // What the service calls an alias is the email address for this Alias
    // object:
    alias_obj->email = *alias;
    aliases.push_back(std::move(alias_obj));
  }
  for (auto& observer : observers_) {
    observer->OnAliasesUpdated(mojo::Clone(aliases));
  }
}

}  // namespace email_aliases
