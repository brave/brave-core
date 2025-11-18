/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_service.h"

#include <memory>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/byte_count.h"
#include "base/check.h"
#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/expected.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_api.h"
#include "brave/components/email_aliases/features.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

namespace {

constexpr char kBraveServicesKeyHeader[] = "Brave-Key";

constexpr char kAccountServiceEndpoint[] = "https://%s/v2/%s";
constexpr char kAccountsServiceVerifyInitPath[] = "verify/init";
constexpr char kAccountsServiceVerifyResultPath[] = "verify/result";

constexpr char kEmailAliasesServiceURL[] = "https://%s/manage";

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

constexpr auto kMaxResponseLength = base::KiB(32);

// Parses a JSON response body into type T (which must expose a `message` field)
// from arbitrary JSON. On success, returns base::ok(T)
// when T parses successfully and T.message == |expected_message|. On failure,
// returns base::unexpected with a user-facing error string, preferring a
// backend-reported error (via ErrorMessage) when available, or a generic
// invalid-response error when the shape is unexpected.
template <typename T>
base::expected<T, std::string> ParseResponseDictAs(
    const std::optional<std::string>& response_body,
    const std::string_view expected_message) {
  if (!response_body) {
    return base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY));
  }
  auto value_opt = base::JSONReader::Read(*response_body);
  if (!value_opt) {
    return base::unexpected(l10n_util::GetStringUTF8(
        IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY));
  }
  auto parsed = T::FromValue(*value_opt);
  if (parsed.has_value() && parsed.value().message == expected_message) {
    return base::ok(std::move(*parsed));
  }
  auto error_message = ErrorMessage::FromValue(*value_opt);
  if (error_message.has_value()) {
    return base::unexpected(
        l10n_util::GetStringFUTF8(IDS_EMAIL_ALIASES_SERVICE_REPORTED_ERROR,
                                  base::UTF8ToUTF16(error_message->message)));
  }
  return base::unexpected(l10n_util::GetStringUTF8(
      IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY));
}

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
GURL EmailAliasesService::GetEmailAliasesServiceURL() {
  return GURL(absl::StrFormat(kEmailAliasesServiceURL,
                              brave_domains::GetServicesDomain("aliases")));
}

EmailAliasesService::EmailAliasesService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory),
      verify_init_url_(GetAccountsServiceVerifyInitURL()),
      verify_result_url_(GetAccountsServiceVerifyResultURL()),
      email_aliases_service_base_url_(GetEmailAliasesServiceURL()) {
  CHECK(base::FeatureList::IsEnabled(email_aliases::features::kEmailAliases));
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
  resource_request->headers.SetHeader(kBraveServicesKeyHeader,
                                      BUILDFLAG(BRAVE_SERVICES_KEY));
  verification_simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kTrafficAnnotation);
  verification_simple_url_loader_->SetRetryOptions(
      /* max_retries=*/3,
      network::SimpleURLLoader::RETRY_ON_5XX |
          network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);
  verification_simple_url_loader_->AttachStringForUpload(*body,
                                                         "application/json");

  // Wrap the mojo result callback for case when user's cancelled verification
  // flow. It is an error to drop response callbacks which still correspond to
  // an open interface pipe.
  auto wrapper = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), base::unexpected(std::string()));
  verification_simple_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&EmailAliasesService::OnRequestAuthenticationResponse,
                     base::Unretained(this), std::move(wrapper)),
      kMaxResponseLength.InBytesUnsigned());
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
  resource_request->headers.SetHeader(kBraveServicesKeyHeader,
                                      BUILDFLAG(BRAVE_SERVICES_KEY));
  verification_simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kTrafficAnnotation);
  verification_simple_url_loader_->AttachStringForUpload(*body,
                                                         "application/json");
  verification_simple_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&EmailAliasesService::OnRequestSessionResponse,
                     base::Unretained(this)),
      kMaxResponseLength.InBytesUnsigned());
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
  base::Value::Dict body_value;  // empty JSON object required by the API
  ApiFetch(email_aliases_service_base_url_,
           net::HttpRequestHeaders::kPostMethod, body_value,
           base::BindOnce(&EmailAliasesService::OnGenerateAliasResponse,
                          weak_factory_.GetWeakPtr(), std::move(callback)));
}

void EmailAliasesService::UpdateAlias(
    const std::string& alias_email,
    const std::optional<std::string>& /* note */,
    UpdateAliasCallback callback) {
  // Build JSON using IDL-defined shape
  UpdateAliasRequest request;
  request.alias = alias_email;
  // TODO(https://github.com/brave/brave-browser/issues/49229):
  // Add support for storing alias note in the client.

  // For now, we only support active aliases.
  request.status = "active";
  auto body_value = request.ToValue();
  ApiFetch(email_aliases_service_base_url_, net::HttpRequestHeaders::kPutMethod,
           body_value,
           base::BindOnce(&EmailAliasesService::OnEditAliasResponse,
                          weak_factory_.GetWeakPtr(), std::move(callback),
                          /*update_expected=*/true));
}

void EmailAliasesService::DeleteAlias(const std::string& alias_email,
                                      DeleteAliasCallback callback) {
  DeleteAliasRequest request;
  request.alias = alias_email;
  auto body_value = request.ToValue();
  ApiFetch(email_aliases_service_base_url_,
           net::HttpRequestHeaders::kDeleteMethod, body_value,
           base::BindOnce(&EmailAliasesService::OnEditAliasResponse,
                          weak_factory_.GetWeakPtr(), std::move(callback),
                          /*update_expected=*/false));
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

void EmailAliasesService::ApiFetch(const GURL& url,
                                   const std::string_view method,
                                   BodyAsStringCallback callback) {
  CHECK(method == net::HttpRequestHeaders::kGetMethod ||
        method == net::HttpRequestHeaders::kHeadMethod);
  ApiFetchInternal(url, method, /*serialized_body=*/std::nullopt,
                   std::move(callback));
}

void EmailAliasesService::ApiFetch(const GURL& url,
                                   const std::string_view method,
                                   const base::Value::Dict& body_value,
                                   BodyAsStringCallback callback) {
  CHECK(method == net::HttpRequestHeaders::kPostMethod ||
        method == net::HttpRequestHeaders::kPutMethod ||
        method == net::HttpRequestHeaders::kDeleteMethod);
  auto body = base::WriteJson(body_value);
  CHECK(body);
  ApiFetchInternal(url, method, /*serialized_body=*/body, std::move(callback));
}

void EmailAliasesService::ApiFetchInternal(
    const GURL& url,
    const std::string_view method,
    std::optional<std::string> serialized_body,
    BodyAsStringCallback callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = method;
  resource_request->headers.SetHeader("Authorization",
                                      std::string("Bearer ") + auth_token_);
  resource_request->headers.SetHeader("X-API-key",
                                      BUILDFLAG(BRAVE_SERVICES_KEY));
  auto simple_url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), kTrafficAnnotation);
  simple_url_loader->SetAllowHttpErrorResults(true);
  if (serialized_body.has_value()) {
    simple_url_loader->AttachStringForUpload(*serialized_body, "text/plain");
  }
  auto* loader_ptr = simple_url_loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(
          [](BodyAsStringCallback inner_callback,
             std::unique_ptr<network::SimpleURLLoader> /*keep_alive_loader*/,
             std::optional<std::string> response_body) {
            std::move(inner_callback).Run(std::move(response_body));
          },
          std::move(callback), std::move(simple_url_loader)),
      kMaxResponseLength.InBytesUnsigned());
}

void EmailAliasesService::OnGenerateAliasResponse(
    GenerateAliasCallback user_callback,
    std::optional<std::string> response_body) {
  auto parsed =
      ParseResponseDictAs<GenerateAliasResponse>(response_body, "created");
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
    std::optional<std::string> response_body) {
  RefreshAliases();
  auto parsed = ParseResponseDictAs<AliasEditedResponse>(
      response_body, update_expected ? "updated" : "deleted");
  auto result =
      parsed.has_value()
          ? base::expected<std::monostate, std::string>(std::monostate{})
          : base::unexpected(parsed.error());
  std::move(user_callback).Run(std::move(result));
}

void EmailAliasesService::RefreshAliases() {
  ApiFetch(email_aliases_service_base_url_.Resolve("?status=active"),
           net::HttpRequestHeaders::kGetMethod,
           base::BindOnce(&EmailAliasesService::OnRefreshAliasesResponse,
                          weak_factory_.GetWeakPtr()));
}

void EmailAliasesService::OnRefreshAliasesResponse(
    std::optional<std::string> response_body) {
  // TODO(https://github.com/brave/brave-browser/issues/48959):
  // In this function, when an error happens, we should show
  // an error message to the user (requires design work)
  if (!response_body) {
    LOG(ERROR) << "Email Aliases service error: No response body";
    return;
  }
  auto parsed = base::JSONReader::Read(*response_body);
  // TODO(https://github.com/brave/brave-browser/issues/49624):
  // Remove this check once the backend is updated to return a dictionary.
  if (parsed && parsed->is_list()) {
    // Wrap the list in a dictionary to match the AliasListResponse shape.
    base::Value::Dict dict;
    dict.Set("result", std::move(*parsed));
    parsed = base::Value(std::move(dict));
  }
  if (!parsed || !parsed->is_dict()) {
    LOG(ERROR) << "Email Aliases service error: Invalid response format";
    return;
  }
  const base::Value::Dict& parsed_dict = parsed->GetDict();
  if (const std::string* error_message = parsed_dict.FindString("message")) {
    LOG(ERROR) << "Email Aliases service error: " << *error_message;
    return;
  }
  auto list_response = AliasListResponse::FromValue(parsed_dict);
  if (!list_response) {
    LOG(ERROR) << "Email Aliases service error: Invalid response format";
    return;
  }
  std::vector<email_aliases::mojom::AliasPtr> aliases;
  for (const auto& entry : list_response->result) {
    auto alias_obj = email_aliases::mojom::Alias::New();
    alias_obj->email = entry.alias;
    aliases.push_back(std::move(alias_obj));
  }
  for (auto& observer : observers_) {
    observer->OnAliasesUpdated(mojo::Clone(aliases));
  }
}

}  // namespace email_aliases
