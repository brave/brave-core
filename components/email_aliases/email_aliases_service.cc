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
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/types/expected.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_api.h"
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

constexpr char kAccountServiceEndpoint[] = "https://%s/v2/%s";
constexpr char kAccountsServiceVerifyInitPath[] = "verify/init";
constexpr char kAccountsServiceVerifyResultPath[] = "verify/result";

// Minimum interval between verify/result polls
constexpr base::TimeDelta kSessionPollInterval = base::Seconds(2);
// Maximum total polling duration for a single verification flow.
constexpr base::TimeDelta kMaxSessionPollDuration = base::Minutes(30);

const net::NetworkTrafficAnnotationTag traffic_annotation =
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

EmailAliasesService::EmailAliasesService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory),
      verify_init_url_(GetAccountsServiceVerifyInitURL()),
      verify_result_url_(GetAccountsServiceVerifyResultURL()) {
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
      std::move(resource_request), traffic_annotation);
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
  const auto response_body_dict = base::JSONReader::ReadDict(
      *response_body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
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
      std::move(resource_request), traffic_annotation);
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
  const auto response_body_dict = base::JSONReader::ReadDict(
      *response_body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
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
  NOTIMPLEMENTED();
  std::move(callback).Run(base::unexpected(std::string("Not implemented")));
}

void EmailAliasesService::UpdateAlias(const std::string& alias_email,
                                      const std::optional<std::string>& note,
                                      UpdateAliasCallback callback) {
  // TODO: Implement alias update logic
  NOTIMPLEMENTED();
  std::move(callback).Run(base::unexpected(std::string("Not implemented")));
}

void EmailAliasesService::DeleteAlias(const std::string& alias_email,
                                      DeleteAliasCallback callback) {
  // TODO: Implement alias deletion logic
  NOTIMPLEMENTED();
  std::move(callback).Run(base::unexpected(std::string("Not implemented")));
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

}  // namespace email_aliases
