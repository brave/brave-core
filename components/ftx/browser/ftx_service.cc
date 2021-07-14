// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ftx/browser/ftx_service.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "brave/components/ftx/browser/ftx_json_parser.h"
#include "brave/components/ftx/common/pref_names.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_oauth.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

namespace {

const char api_host[] = "ftx.com";
const char oauth_callback[] = "com.brave.ftx://authorization";
const unsigned int kRetriesCountOnNetworkChange = 1;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ftx_service", R"(
      semantics {
        sender: "FTX Service"
        description:
          "This service is used to communicate with FTX "
          "on behalf of the user interacting with the FTX widget."
        trigger:
          "Triggered by using the FTX widget."
        data:
          "Account balance for the widget."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on the new tab page."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

GURL GetURLWithPath(const std::string& host, const std::string& path) {
  return GURL(std::string(url::kHttpsScheme) + "://" + host).Resolve(path);
}

void BuildFormEncoding(const std::string& key,
                       const std::string& value,
                       std::string* out) {
  if (!out->empty())
    out->append("&");
  out->append(key + "=" + net::EscapeUrlEncodedData(value, true));
}

}  // namespace

FTXService::FTXService(content::BrowserContext* context)
    : client_id_(FTX_CLIENT_ID),
      client_secret_(FTX_CLIENT_SECRET),
      context_(context),
      url_loader_factory_(context_->GetDefaultStoragePartition()
                              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  // Get access token from prefs
  std::string encoded_encrypted_access_token =
      prefs->GetString(kFTXAccessToken);
  std::string encrypted_access_token;
  if (!base::Base64Decode(encoded_encrypted_access_token,
                          &encrypted_access_token)) {
    LOG(ERROR) << "FTX: Could not decode Token info from prefs";
  } else {
    if (!OSCrypt::DecryptString(encrypted_access_token, &access_token_)) {
      LOG(ERROR) << "FTX: Could not decrypt and save access token";
    }
  }
}

FTXService::~FTXService() {}

void FTXService::Shutdown() {
  url_loaders_.clear();
}

bool FTXService::GetFuturesData(GetFuturesDataCallback callback) {
  auto internal_callback = base::BindOnce(
      &FTXService::OnFuturesData, base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, get_futures_data_path);
  return NetworkRequest(url, "GET", "", "",
      std::move(internal_callback), false);
}

void FTXService::OnFuturesData(
    GetFuturesDataCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  FTXFuturesData data;
  if (status >= 200 && status <= 299) {
    FTXJSONParser::GetFuturesDataFromJSON(body, &data, futures_filter);
  }
  std::move(callback).Run(data);
}

bool FTXService::GetChartData(const std::string& symbol,
                              const std::string& start,
                              const std::string& end,
                              GetChartDataCallback callback) {
  auto internal_callback = base::BindOnce(
      &FTXService::OnChartData, base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(
      api_host, std::string(get_market_data_path) + "/" + symbol + "/candles");
  url = net::AppendQueryParameter(url, "resolution", "14400");
  url = net::AppendQueryParameter(url, "limit", "42");
  if (!start.empty()) {
    url = net::AppendQueryParameter(url, "start_time", start);
  }
  if (!end.empty()) {
    url = net::AppendQueryParameter(url, "end_time", end);
  }
  return NetworkRequest(url, "GET", "", "",
      std::move(internal_callback), false);
}

void FTXService::OnChartData(
    GetChartDataCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  FTXChartData data;
  if (status >= 200 && status <= 299) {
    FTXJSONParser::GetChartDataFromJSON(body, &data);
  }
  std::move(callback).Run(data);
}

bool FTXService::GetAccountBalances(GetAccountBalancesCallback callback) {
  auto internal_callback =
      base::BindOnce(&FTXService::OnGetAccountBalances, base::Unretained(this),
                     std::move(callback));
  GURL url = GetOAuthURL(oauth_balances_path);
  return NetworkRequest(url, "GET", "", "", std::move(internal_callback), true);
}

void FTXService::OnGetAccountBalances(
    GetAccountBalancesCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  FTXAccountBalances balances;
  bool auth_invalid = status == 401;
  if (status >= 200 && status <= 299) {
    FTXJSONParser::GetAccountBalancesFromJSON(body, &balances);
  }
  std::move(callback).Run(balances, auth_invalid);
}

GURL FTXService::GetOAuthURL(const std::string& path) {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  std::string oauth_host = prefs->GetString(kFTXOauthHost);
  return GURL(std::string(url::kHttpsScheme) + "://" + oauth_host)
      .Resolve(path);
}

std::string FTXService::GetTokenHeader() {
  if (access_token_.empty()) {
    return "Basic " + client_id_ + ":" + client_secret_;
  } else {
    return "Bearer " + access_token_;
  }
}

std::string FTXService::GetOAuthClientUrl() {
  // This particular FTX Url has a strange format. It is parameterized as if
  // it has a query param, except the params are the last path segment.
  auto state = ntp_widget_utils::GetCryptoRandomString(false);
  std::string path = std::string(oauth_path) + "/response_type=code" +
      "&client_id=" + net::EscapeQueryParamValue(client_id_, true) +
      "&state=" + net::EscapeQueryParamValue(state, true) +
      "&redirect_uri=" + net::EscapeQueryParamValue(oauth_callback, true);
  GURL url = GetOAuthURL(path);
  return url.spec();
}

bool FTXService::AuthenticateFromAuthToken(const std::string& auth_token) {
  GURL url = GetOAuthURL(oauth_token_path);
  std::string body;
  // This is the only API POST that needs to be in form type.
  BuildFormEncoding("grant_type", "code", &body);
  BuildFormEncoding("redirect_uri", oauth_callback, &body);
  BuildFormEncoding("code", auth_token, &body);
  access_token_.clear();
  // Handle response from API network call
  auto onRequest = base::BindOnce(
      [](FTXService* service,
          const int status, const std::string& body,
          const std::map<std::string, std::string>& headers) {
      std::string access_token;
      if (status >= 200 && status <= 299) {
        if (FTXJSONParser::GetAccessTokenFromJSON(body, &access_token)) {
          service->SetAccessToken(access_token);
        } else {
          LOG(ERROR) << "ftx: unable to parse access token";
        }
      } else {
        LOG(ERROR) << "ftx: bad access token status" << status;
      }
    }, base::Unretained(this));

  return NetworkRequest(url, "POST", body, "application/x-www-form-urlencoded",
      std::move(onRequest), true);
}

bool FTXService::GetConvertQuote(
    const std::string& from,
    const std::string& to,
    const std::string& amount,
    GetConvertQuoteCallback callback) {
  auto internal_callback = base::BindOnce(&FTXService::OnGetConvertQuote,
      base::Unretained(this), std::move(callback));
  GURL url = GetOAuthURL(oauth_quote_path);
  base::Value request_data(base::Value::Type::DICTIONARY);
  request_data.SetStringKey("fromCoin", from);
  request_data.SetStringKey("toCoin", to);
  request_data.SetStringKey("size", amount);
  std::string body;
  if (!base::JSONWriter::Write(request_data, &body)) {
    LOG(ERROR) << "FTX: Could not serialize convert quote body data!";
    std::move(callback).Run("");
    return false;
  }
  return NetworkRequest(url, "POST", body, "application/json",
      std::move(internal_callback), true);
}

void FTXService::OnGetConvertQuote(
    GetConvertQuoteCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string quote_id;
  if (status >= 200 && status <= 299) {
    FTXJSONParser::GetQuoteIdJSON(body, &quote_id);
  }
  std::move(callback).Run(quote_id);
}

bool FTXService::GetConvertQuoteInfo(const std::string& quote_id,
                                     GetConvertQuoteInfoCallback callback) {
  auto internal_callback = base::BindOnce(&FTXService::OnGetConvertQuoteInfo,
      base::Unretained(this), std::move(callback));
  GURL url = GetOAuthURL(std::string(oauth_quote_path) + "/" + quote_id);
  return NetworkRequest(url, "GET", "", "", std::move(internal_callback), true);
}

void FTXService::OnGetConvertQuoteInfo(
    GetConvertQuoteInfoCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string price;
  std::string cost;
  std::string proceeds;
  if (status >= 200 && status <= 299) {
    FTXJSONParser::GetQuoteStatusJSON(body, &cost, &price, &proceeds);
  }
  std::move(callback).Run(cost, price, proceeds);
}

bool FTXService::ExecuteConvertQuote(const std::string& quote_id,
                                     ExecuteConvertQuoteCallback callback) {
  auto internal_callback = base::BindOnce(&FTXService::OnExecuteConvertQuote,
      base::Unretained(this), std::move(callback));
  GURL url = GetOAuthURL(
      std::string(oauth_quote_path) + "/" + quote_id + "/accept");
  return NetworkRequest(url, "POST", "", "",
      std::move(internal_callback), true);
}

void FTXService::OnExecuteConvertQuote(
    ExecuteConvertQuoteCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::move(callback).Run(status >= 200 && status <= 299);
}

bool FTXService::SetAccessToken(const std::string& access_token) {
  access_token_ = access_token;
  std::string encrypted_access_token;

  if (!OSCrypt::EncryptString(access_token, &encrypted_access_token)) {
    LOG(ERROR) << "Could not encrypt and save FTX token info";
    return false;
  }

  std::string encoded_encrypted_access_token;
  base::Base64Encode(encrypted_access_token, &encoded_encrypted_access_token);

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  prefs->SetString(kFTXAccessToken, encoded_encrypted_access_token);

  return true;
}

void FTXService::ClearAuth() {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  access_token_.clear();
  prefs->ClearPref(kFTXAccessToken);
}

bool FTXService::NetworkRequest(const GURL& url,
                                const std::string& method,
                                const std::string& post_data,
                                const std::string& post_data_type,
                                URLRequestCallback callback,
                                bool set_auth_header) {
  auto request = std::make_unique<network::ResourceRequest>();
  if (set_auth_header) {
    std::string header = GetTokenHeader();
    request->headers.SetHeader(net::HttpRequestHeaders::kAuthorization,
         base::StringPrintf("%s", header.c_str()));
  }
  request->url = url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SAVE_COOKIES;
  request->method = method;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  if (!post_data.empty() && !post_data_type.empty()) {
    url_loader->AttachStringForUpload(post_data, post_data_type);
  }
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  auto* default_storage_partition = context_->GetDefaultStoragePartition();
  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory,
      base::BindOnce(&FTXService::OnURLLoaderComplete, base::Unretained(this),
                    iter, std::move(callback)));

  return true;
}

void FTXService::OnURLLoaderComplete(
    SimpleURLLoaderList::iterator iter,
    URLRequestCallback callback,
    const std::unique_ptr<std::string> response_body) {
  auto* loader = iter->get();
  auto response_code = -1;
  std::map<std::string, std::string> headers;
  if (loader->ResponseInfo() && loader->ResponseInfo()->headers) {
    response_code = loader->ResponseInfo()->headers->response_code();
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headers_list->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
      }
    }
  }

  url_loaders_.erase(iter);

  std::move(callback).Run(response_code, response_body ? *response_body : "",
                          headers);
}

base::SequencedTaskRunner* FTXService::io_task_runner() {
  if (!io_task_runner_) {
    io_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}
