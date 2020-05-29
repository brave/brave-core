/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/access_token_fetcher_impl.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_status.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_sync {

namespace {

constexpr char kExpiresInKey[] = "expires_in";
constexpr char kErrorKey[] = "error";
constexpr char kTimestampKey[] = "timestamp";

constexpr char kTimestampSuffix[] = "v2/timestamp";

// Enumerated constants for logging server responses on 400 errors, matching
// RFC 6749.
enum OAuth2ErrorCodesForHistogram {
  OAUTH2_ACCESS_ERROR_INVALID_REQUEST = 0,
  OAUTH2_ACCESS_ERROR_INVALID_CLIENT,
  OAUTH2_ACCESS_ERROR_INVALID_GRANT,
  OAUTH2_ACCESS_ERROR_UNAUTHORIZED_CLIENT,
  OAUTH2_ACCESS_ERROR_UNSUPPORTED_GRANT_TYPE,
  OAUTH2_ACCESS_ERROR_INVALID_SCOPE,
  OAUTH2_ACCESS_ERROR_UNKNOWN,
  OAUTH2_ACCESS_ERROR_COUNT
};

OAuth2ErrorCodesForHistogram OAuth2ErrorToHistogramValue(
    const std::string& error) {
  if (error == "invalid_request")
    return OAUTH2_ACCESS_ERROR_INVALID_REQUEST;
  else if (error == "invalid_client")
    return OAUTH2_ACCESS_ERROR_INVALID_CLIENT;
  else if (error == "invalid_grant")
    return OAUTH2_ACCESS_ERROR_INVALID_GRANT;
  else if (error == "unauthorized_client")
    return OAUTH2_ACCESS_ERROR_UNAUTHORIZED_CLIENT;
  else if (error == "unsupported_grant_type")
    return OAUTH2_ACCESS_ERROR_UNSUPPORTED_GRANT_TYPE;
  else if (error == "invalid_scope")
    return OAUTH2_ACCESS_ERROR_INVALID_SCOPE;

  return OAUTH2_ACCESS_ERROR_UNKNOWN;
}

static GoogleServiceAuthError CreateAuthError(int net_error) {
  CHECK_NE(net_error, net::OK);
  DLOG(WARNING) << "Server error: errno " << net_error;
  return GoogleServiceAuthError::FromConnectionError(net_error);
}

static std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(
    const GURL& url,
    const std::string& body) {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("sync_access_token_fetcher", R"(
        semantics {
          sender: "Brave Sync Access Token Fetcher"
          description:
            "This request is used by the ProfileSyncService to fetch "
            "access token for a sync chain"
          trigger:
            "This request can be triggered at any moment when
            "ProfileSyncService requests an access token"
          data:
            "none"
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled in settings"
          chrome_policy {
            SigninAllowed {
              policy_options {mode: MANDATORY}
              SigninAllowed: false
            }
          }
        })");

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  if (!body.empty())
    resource_request->method = "POST";

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);

  if (!body.empty())
    url_loader->AttachStringForUpload(body,
                                      "application/x-www-form-urlencoded");

  // We want to receive the body even on error, as it may contain the reason for
  // failure.
  url_loader->SetAllowHttpErrorResults(true);

  // Fetchers are sometimes cancelled because a network change was detected,
  // especially at startup and after sign-in on ChromeOS. Retrying once should
  // be enough in those cases; let the fetcher retry up to 3 times just in case.
  // http://crbug.com/163710
  url_loader->SetRetryOptions(
      3, network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);

  return url_loader;
}

std::unique_ptr<base::DictionaryValue> ParseServerResponse(
    std::unique_ptr<std::string> data) {
  if (!data)
    return nullptr;

  std::unique_ptr<base::Value> value = base::JSONReader::ReadDeprecated(*data);
  if (!value.get() || value->type() != base::Value::Type::DICTIONARY)
    value.reset();

  return std::unique_ptr<base::DictionaryValue>(
      static_cast<base::DictionaryValue*>(value.release()));
}

std::string GenerateAccessToken(const std::vector<uint8_t>& public_key,
                                const std::vector<uint8_t>& private_key,
                                const std::string& timestamp) {
  const std::string public_key_hex =
      base::HexEncode(public_key.data(), public_key.size());

  const std::string timestamp_hex =
      base::HexEncode(timestamp.data(), timestamp.size());

  std::vector<uint8_t> timestamp_bytes;
  base::HexStringToBytes(timestamp_hex, &timestamp_bytes);
  std::vector<uint8_t> signature;
  brave_sync::crypto::Sign(timestamp_bytes, private_key, &signature);
  DCHECK(brave_sync::crypto::Verify(timestamp_bytes, signature, public_key));

  const std::string signed_timestamp_hex =
      base::HexEncode(signature.data(), signature.size());

  VLOG(1) << "timestamp_hex= " << timestamp_hex;
  VLOG(1) << "signed_timestamp_hex= " << signed_timestamp_hex;
  VLOG(1) << "public_key_hex= " << public_key_hex;

  // base64(timestamp_hex|signed_timestamp_hex|public_key_hex)
  const std::string access_token =
      timestamp_hex + "|" + signed_timestamp_hex + "|" + public_key_hex;
  std::string encoded_access_token;
  base::Base64Encode(access_token, &encoded_access_token);
  DCHECK(!encoded_access_token.empty());
  return encoded_access_token;
}

}  // namespace

AccessTokenFetcherImpl::AccessTokenFetcherImpl(
    AccessTokenConsumer* consumer,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const GURL& sync_service_url)
    : AccessTokenFetcher(consumer),
      url_loader_factory_(url_loader_factory),
      sync_service_url_(sync_service_url) {}

AccessTokenFetcherImpl::~AccessTokenFetcherImpl() {}

void AccessTokenFetcherImpl::CancelRequest() {
  url_loader_.reset();
}

void AccessTokenFetcherImpl::Start(const std::vector<uint8_t>& public_key,
                                   const std::vector<uint8_t>& private_key) {
  url_loader_ = CreateURLLoader(MakeGetTimestampUrl(), "");
  // It's safe to use Unretained below as the |url_loader_| is owned by |this|.
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&AccessTokenFetcherImpl::OnURLLoadComplete,
                     base::Unretained(this), public_key, private_key),
      1024 * 1024);
}

void AccessTokenFetcherImpl::OnURLLoadComplete(
    const std::vector<uint8_t>& public_key,
    const std::vector<uint8_t>& private_key,
    std::unique_ptr<std::string> response_body) {
  int histogram_value;
  bool net_failure = IsNetFailure(url_loader_.get(), &histogram_value);
  base::UmaHistogramSparse(
      "BraveSync.AccessTokenFetcherImpl.TimestampResponseCode",
      histogram_value);
  if (net_failure) {
    OnGetTokenFailure(CreateAuthError(histogram_value));
    return;
  }

  int response_code = url_loader_->ResponseInfo()->headers->response_code();
  switch (response_code) {
    case net::HTTP_OK:
      break;
    case net::HTTP_PROXY_AUTHENTICATION_REQUIRED:
      NOTREACHED() << "HTTP 407 should be treated as a network error.";
      // If this ever happens in production, we treat it as a temporary error as
      // it is similar to a network error.
      OnGetTokenFailure(
          GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
      return;
    case net::HTTP_FORBIDDEN:
      // HTTP_FORBIDDEN (403) is treated as temporary error, because it may be
      // '403 Rate Limit Exeeded.'
      OnGetTokenFailure(
          GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
      return;
    case net::HTTP_BAD_REQUEST: {
      // HTTP_BAD_REQUEST (400) usually contains error as per
      // http://tools.ietf.org/html/rfc6749#section-5.2.
      std::string gaia_error;
      if (!ParseGetTimestampFailureResponse(std::move(response_body),
                                            &gaia_error)) {
        OnGetTokenFailure(
            GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR));
        return;
      }
      OAuth2ErrorCodesForHistogram access_error(
          OAuth2ErrorToHistogramValue(gaia_error));

      OnGetTokenFailure(
          access_error == OAUTH2_ACCESS_ERROR_INVALID_GRANT
              ? GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
                    GoogleServiceAuthError::InvalidGaiaCredentialsReason::
                        CREDENTIALS_REJECTED_BY_SERVER)
              : GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR));
      return;
    }
    default: {
      if (response_code >= net::HTTP_INTERNAL_SERVER_ERROR) {
        // 5xx is always treated as transient.
        OnGetTokenFailure(GoogleServiceAuthError(
            GoogleServiceAuthError::SERVICE_UNAVAILABLE));
      } else {
        // The other errors are treated as permanent error.
        DLOG(ERROR) << "Unexpected persistent error: http_status="
                    << response_code;
        OnGetTokenFailure(
            GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
                GoogleServiceAuthError::InvalidGaiaCredentialsReason::
                    CREDENTIALS_REJECTED_BY_SERVER));
      }
      return;
    }
  }

  // The request was successfully fetched and it returned OK.
  // Parse out the access token and the expiration time.
  std::string timestamp;
  int expires_in;
  if (!ParseGetTimestampSuccessResponse(std::move(response_body), &timestamp,
                                        &expires_in)) {
    DLOG(WARNING) << "Response doesn't match expected format";
    OnGetTokenFailure(
        GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
    return;
  }

  std::string access_token =
      GenerateAccessToken(public_key, private_key, timestamp);
  // The token will expire in |expires_in| seconds. Take a 10% error margin to
  // prevent reusing a token too close to its expiration date.
  OnGetTokenSuccess(AccessTokenConsumer::TokenResponse(
      access_token,
      base::Time::Now() + base::TimeDelta::FromSeconds(9 * expires_in / 10)));
}

void AccessTokenFetcherImpl::OnGetTokenSuccess(
    const AccessTokenConsumer::TokenResponse& token_response) {
  FireOnGetTokenSuccess(token_response);
}

void AccessTokenFetcherImpl::OnGetTokenFailure(
    const GoogleServiceAuthError& error) {
  FireOnGetTokenFailure(error);
}

bool AccessTokenFetcherImpl::IsNetFailure(network::SimpleURLLoader* loader,
                                          int* histogram_value) {
  DCHECK(loader);
  DCHECK(histogram_value);
  if (loader->NetError() == net::OK && loader->ResponseInfo() &&
      loader->ResponseInfo()->headers) {
    *histogram_value = loader->ResponseInfo()->headers->response_code();
    return false;
  }
  *histogram_value = loader->NetError();
  return true;
}

GURL AccessTokenFetcherImpl::MakeGetTimestampUrl() {
  return sync_service_url_.Resolve(kTimestampSuffix);
}

// static
bool AccessTokenFetcherImpl::ParseGetTimestampSuccessResponse(
    std::unique_ptr<std::string> response_body,
    std::string* timestamp,
    int* expires_in) {
  CHECK(timestamp);
  std::unique_ptr<base::DictionaryValue> value =
      ParseServerResponse(std::move(response_body));
  if (!value)
    return false;
  return value->GetString(kTimestampKey, timestamp) &&
         value->GetInteger(kExpiresInKey, expires_in);
}

// static
bool AccessTokenFetcherImpl::ParseGetTimestampFailureResponse(
    std::unique_ptr<std::string> response_body,
    std::string* error) {
  CHECK(error);
  std::unique_ptr<base::DictionaryValue> value =
      ParseServerResponse(std::move(response_body));
  return value ? value->GetString(kErrorKey, error) : false;
}

}  // namespace brave_sync
