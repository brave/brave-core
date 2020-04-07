#include "brave/components/brave_sync/access_token_fetcher_impl.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_status.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_sync {

namespace {
constexpr char kGetAccessTokenBodyFormat[] =
    "client_id=%s&"
    "client_secret=%s&"
    "timestamp=%s&"
    "refresh_token=%s";

constexpr char kAccessTokenKey[] = "access_token";
constexpr char kExpiresInKey[] = "expires_in";
constexpr char kIdTokenKey[] = "id_token";
constexpr char kErrorKey[] = "error";
constexpr char kTimestamp[] = "timestamp";

static std::string CreateAuthError(int net_error) {
  CHECK_NE(net_error, net::OK);
  DLOG(WARNING) << "Server error: errno "
                << net_error;
  return net::ErrorToString(net_error);
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
            "Brave Sync client id and secrect and refresh token"
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

std::unique_ptr<base::DictionaryValue> ParseGetAccessTokenResponse(
    std::unique_ptr<std::string> data) {
  if (!data)
    return nullptr;

  std::unique_ptr<base::Value> value = base::JSONReader::ReadDeprecated(*data);
  if (!value.get() || value->type() != base::Value::Type::DICTIONARY)
    value.reset();

  return std::unique_ptr<base::DictionaryValue>(
      static_cast<base::DictionaryValue*>(value.release()));
}

}  // namespace

AccessTokenFetcherImpl::AccessTokenFetcherImpl(
    AccessTokenConsumer* consumer,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const std::string& refresh_token)
    : AccessTokenFetcher(consumer),
      url_loader_factory_(url_loader_factory),
      refresh_token_(refresh_token),
      state_(INITIAL) {}

AccessTokenFetcherImpl::~AccessTokenFetcherImpl() {}

void AccessTokenFetcherImpl::CancelRequest() {
  url_loader_.reset();
  ts_url_loader_.reset();
}

void AccessTokenFetcherImpl::Start(
    const std::string& client_id,
    const std::string& client_secret,
    const std::string& timestamp) {
  client_id_ = client_id;
  client_secret_ = client_secret;
  timestamp_ = timestamp;
  StartGetAccessToken();
}

void AccessTokenFetcherImpl::StartGetTimestamp() {
  ts_url_loader_ =
      CreateURLLoader(GURL("http://localhost:8295/v2/timestamp"),
                      "");
  // It's safe to use Unretained below as the |url_loader_| is owned by |this|.
  ts_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&AccessTokenFetcherImpl::OnTimestampLoadComplete,
                     base::Unretained(this)),
      1024 * 1024);
}

void AccessTokenFetcherImpl::StartGetAccessToken() {
  CHECK_EQ(INITIAL, state_);
  state_ = GET_ACCESS_TOKEN_STARTED;
  url_loader_ =
      CreateURLLoader(MakeGetAccessTokenUrl(),
                      MakeGetAccessTokenBody(client_id_, client_secret_,
                                             timestamp_, refresh_token_));
  // It's safe to use Unretained below as the |url_loader_| is owned by |this|.
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&AccessTokenFetcherImpl::OnURLLoadComplete,
                     base::Unretained(this)),
      1024 * 1024);
}

void AccessTokenFetcherImpl::EndGetAccessToken(
    std::unique_ptr<std::string> response_body) {
  CHECK_EQ(GET_ACCESS_TOKEN_STARTED, state_);
  state_ = GET_ACCESS_TOKEN_DONE;

  bool net_failure = false;
  int histogram_value;
  if (url_loader_->NetError() == net::OK && url_loader_->ResponseInfo() &&
      url_loader_->ResponseInfo()->headers) {
    histogram_value = url_loader_->ResponseInfo()->headers->response_code();
  } else {
    histogram_value = url_loader_->NetError();
    net_failure = true;
  }
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
      OnGetTokenFailure(net::ErrorToString(response_code));
      return;
    case net::HTTP_FORBIDDEN:
      // HTTP_FORBIDDEN (403) is treated as temporary error, because it may be
      // '403 Rate Limit Exeeded.'
      OnGetTokenFailure(net::ErrorToString(response_code));
      return;
    case net::HTTP_BAD_REQUEST: {
      // HTTP_BAD_REQUEST (400) usually contains error as per
      // http://tools.ietf.org/html/rfc6749#section-5.2.
      std::string gaia_error;
      if (!ParseGetAccessTokenFailureResponse(std::move(response_body),
                                              &gaia_error)) {
        OnGetTokenFailure(net::ErrorToString(response_code));
        return;
      }

      // ErrorCodesForHistogram access_error(
      //     ErrorToHistogramValue(gaia_error));
      // UMA_HISTOGRAM_ENUMERATION("Gaia.BadRequestTypeForAccessToken",
      //                           access_error,
      //                           OAUTH2_ACCESS_ERROR_COUNT);

      // OnGetTokenFailure(
      //     access_error == OAUTH2_ACCESS_ERROR_INVALID_GRANT
      //         ? GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
      //               GoogleServiceAuthError::InvalidGaiaCredentialsReason::
      //                   CREDENTIALS_REJECTED_BY_SERVER)
      //         : GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR));
      return;
    }
    default: {
      if (response_code >= net::HTTP_INTERNAL_SERVER_ERROR) {
        // 5xx is always treated as transient.
        OnGetTokenFailure(net::ErrorToString(response_code));
      } else {
        // The other errors are treated as permanent error.
        DLOG(ERROR) << "Unexpected persistent error: http_status="
                    << response_code;
        OnGetTokenFailure(net::ErrorToString(response_code));
      }
      return;
    }
  }

  // The request was successfully fetched and it returned OK.
  // Parse out the access token and the expiration time.
  std::string access_token;
  int expires_in;
  std::string id_token;
  if (!ParseGetAccessTokenSuccessResponse(
          std::move(response_body), &access_token, &expires_in, &id_token)) {
    DLOG(WARNING) << "Response doesn't match expected format";
    OnGetTokenFailure("Service Unavailable");
    return;
  }
  // The token will expire in |expires_in| seconds. Take a 10% error margin to
  // prevent reusing a token too close to its expiration date.
  OnGetTokenSuccess(AccessTokenConsumer::TokenResponse(
      access_token,
      base::Time::Now() + base::TimeDelta::FromSeconds(9 * expires_in / 10),
      id_token));
}

void AccessTokenFetcherImpl::OnGetTokenSuccess(
    const AccessTokenConsumer::TokenResponse& token_response) {
  FireOnGetTokenSuccess(token_response);
}

void AccessTokenFetcherImpl::OnGetTokenFailure(
    const std::string& error) {
  state_ = ERROR_STATE;
  FireOnGetTokenFailure(error);
}

void AccessTokenFetcherImpl::OnURLLoadComplete(
    std::unique_ptr<std::string> response_body) {
  CHECK(state_ == GET_ACCESS_TOKEN_STARTED);
  EndGetAccessToken(std::move(response_body));
}

void AccessTokenFetcherImpl::OnTimestampLoadComplete(
    std::unique_ptr<std::string> response_body) {
  bool net_failure = false;
  int histogram_value;
  if (ts_url_loader_->NetError() == net::OK && ts_url_loader_->ResponseInfo() &&
      ts_url_loader_->ResponseInfo()->headers) {
    histogram_value = ts_url_loader_->ResponseInfo()->headers->response_code();
  } else {
    histogram_value = ts_url_loader_->NetError();
    net_failure = true;
  }
  if (net_failure) {
    FireOnGetTimestampFailure(CreateAuthError(histogram_value));
    return;
  }
  std::unique_ptr<base::DictionaryValue> value =
      ParseGetAccessTokenResponse(std::move(response_body));
  if (!value)
    return;
  std::string timestamp;
  if (!value->GetString(kTimestamp, &timestamp)) {
    FireOnGetTimestampFailure("Unable to parse timestamp");
    return;
  }
  FireOnGetTimestampSuccess(timestamp);
}

// static
GURL AccessTokenFetcherImpl::MakeGetAccessTokenUrl() {
  return GURL("http://localhost:8295/v2/auth");
}

// static
std::string AccessTokenFetcherImpl::MakeGetAccessTokenBody(
    const std::string& client_id,
    const std::string& client_secret,
    const std::string& timestamp,
    const std::string& refresh_token) {
  std::string enc_client_id = net::EscapeUrlEncodedData(client_id, true);
  std::string enc_client_secret =
      net::EscapeUrlEncodedData(client_secret, true);
  std::string enc_timestamp =
      net::EscapeUrlEncodedData(timestamp, true);
  std::string enc_refresh_token =
      net::EscapeUrlEncodedData(refresh_token, true);
  return base::StringPrintf(kGetAccessTokenBodyFormat,
                            enc_client_id.c_str(),
                            enc_client_secret.c_str(),
                            enc_timestamp.c_str(),
                            enc_refresh_token.c_str());
}

// static
bool AccessTokenFetcherImpl::ParseGetAccessTokenSuccessResponse(
    std::unique_ptr<std::string> response_body,
    std::string* access_token,
    int* expires_in,
    std::string* id_token) {
  CHECK(access_token);
  std::unique_ptr<base::DictionaryValue> value =
      ParseGetAccessTokenResponse(std::move(response_body));
  if (!value)
    return false;
  // ID token field is optional.
  value->GetString(kIdTokenKey, id_token);
  return value->GetString(kAccessTokenKey, access_token) &&
         value->GetInteger(kExpiresInKey, expires_in);
}

// static
bool AccessTokenFetcherImpl::ParseGetAccessTokenFailureResponse(
    std::unique_ptr<std::string> response_body,
    std::string* error) {
  CHECK(error);
  std::unique_ptr<base::DictionaryValue> value =
      ParseGetAccessTokenResponse(std::move(response_body));
  return value ? value->GetString(kErrorKey, error) : false;
}

}   // namespace brave_sync
