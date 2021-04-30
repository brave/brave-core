/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/gemini/browser/gemini_service.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/token.h"
#include "brave/components/gemini/browser/gemini_json_parser.h"
#include "brave/components/gemini/browser/pref_names.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_oauth.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

namespace {
  const char oauth_host[] = "exchange.gemini.com";
  const char api_host[] = "api.gemini.com";
  const char oauth_callback[] = "com.brave.gemini://authorization";
  const char oauth_scope[] = "addresses:read,balances:read,orders:create";
  const char oauth_url[] = "https://exchange.gemini.com/auth";
  const unsigned int kRetriesCountOnNetworkChange = 1;

  net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
    return net::DefineNetworkTrafficAnnotation("gemini_service", R"(
        semantics {
          sender: "Gemini Service"
          description:
            "This service is used to communicate with Gemini "
            "on behalf of the user interacting with the Gemini widget."
          trigger:
            "Triggered by user connecting the Gemini widget."
          data:
            "Account information, balances"
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

  std::string CreateJSONRequestBody(const base::Value& dict) {
    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  std::string GetEncodedRequestPayload(std::string payload) {
    std::string json;
    std::string encoded_payload;
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey("request", payload);
    base::JSONWriter::Write(dict, &json);
    base::Base64Encode(json, &encoded_payload);
    return encoded_payload;
  }

  std::string GetEncodedExecutePayload(std::string symbol,
                                       std::string side,
                                       std::string quantity,
                                       std::string price,
                                       std::string fee,
                                       int quote_id) {
    std::string json;
    std::string encoded_payload;
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey("request",
      std::string(api_path_execute_quote));
    dict.SetStringKey("symbol", symbol);
    dict.SetStringKey("side", side);
    dict.SetStringKey("quantity", quantity);
    dict.SetStringKey("price", price);
    dict.SetStringKey("fee", fee);
    dict.SetIntKey("quoteId", quote_id);
    base::JSONWriter::Write(dict, &json);
    base::Base64Encode(json, &encoded_payload);
    return encoded_payload;
  }

  void CalculateSaleAmount(const std::string quantity,
                           const std::string price,
                           const std::string fee,
                           std::string* total_price) {
    if (quantity.empty() || price.empty() || fee.empty()) {
      return;
    }

    double parsed_quantity;
    double parsed_price;
    double parsed_fee;

    if (!base::StringToDouble(quantity, &parsed_quantity) ||
        !base::StringToDouble(price, &parsed_price) ||
        !base::StringToDouble(fee, &parsed_fee)) {
      return;
    }

    // Sale amount is (quantity * price) - fee
    double sale_amount = (parsed_quantity * parsed_price) - parsed_fee;
    *total_price = std::to_string(sale_amount);
  }

}  // namespace

GeminiService::GeminiService(content::BrowserContext* context)
    : client_id_(GEMINI_CLIENT_ID),
      client_secret_(GEMINI_CLIENT_SECRET),
      oauth_host_(oauth_host),
      api_host_(api_host),
      context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
  LoadTokensFromPrefs();
}

GeminiService::~GeminiService() {
}

std::string GeminiService::GetOAuthClientUrl() {
  GURL url(oauth_url);
  code_verifier_ = ntp_widget_utils::GetCryptoRandomString(false);
  code_challenge_ = ntp_widget_utils::GetCodeChallenge(code_verifier_, false);
  url = net::AppendQueryParameter(url, "response_type", "code");
  url = net::AppendQueryParameter(url, "client_id", client_id_);
  url = net::AppendQueryParameter(url, "redirect_uri", oauth_callback);
  url = net::AppendQueryParameter(url, "scope", oauth_scope);
  url = net::AppendQueryParameter(url, "code_challenge", code_challenge_);
  url = net::AppendQueryParameter(url, "code_challenge_method", "S256");
  url = net::AppendQueryParameter(
      url, "state", ntp_widget_utils::GetCryptoRandomString(false));
  return url.spec();
}

void GeminiService::SetAuthToken(const std::string& auth_token) {
  auth_token_ = auth_token;
}

bool GeminiService::GetAccessToken(AccessTokenCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnGetAccessToken,
      base::Unretained(this), std::move(callback));
  GURL base_url = GetURLWithPath(oauth_host_, auth_path_access_token);

  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("client_id", client_id_);
  dict.SetStringKey("client_secret", client_secret_);
  dict.SetStringKey("code", auth_token_);
  dict.SetStringKey("redirect_uri", oauth_callback);
  dict.SetStringKey("code_verifier", code_verifier_);
  dict.SetStringKey("grant_type", "authorization_code");
  std::string request_body = CreateJSONRequestBody(dict);

  auth_token_.clear();
  return OAuthRequest(
      base_url, "POST", request_body,
      std::move(internal_callback), true, false, "");
}

bool GeminiService::RefreshAccessToken(AccessTokenCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnGetAccessToken,
      base::Unretained(this), std::move(callback));
  GURL base_url = GetURLWithPath(oauth_host_, auth_path_access_token);

  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("client_id", client_id_);
  dict.SetStringKey("client_secret", client_secret_);
  dict.SetStringKey("refresh_token", refresh_token_);
  dict.SetStringKey("grant_type", "refresh_token");
  std::string request_body = CreateJSONRequestBody(dict);

  auth_token_.clear();
  return OAuthRequest(
      base_url, "POST", request_body,
      std::move(internal_callback), true, false, "");
}

void GeminiService::OnGetAccessToken(
    AccessTokenCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string access_token;
  std::string refresh_token;
  if (status >= 200 && status <= 299) {
    GeminiJSONParser::GetTokensFromJSON(body, &access_token, &refresh_token);
    SetAccessTokens(access_token, refresh_token);
  }
  std::move(callback).Run(!access_token.empty() && !refresh_token.empty());
}

bool GeminiService::GetTickerPrice(const std::string& asset,
                                   GetTickerPriceCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnTickerPrice,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host_,
    std::string(api_path_ticker_price) + "/" + asset);
  return OAuthRequest(
      url, "GET", "", std::move(internal_callback), true, false, "");
}

void GeminiService::OnTickerPrice(
  GetTickerPriceCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  std::string price;
  if (status >= 200 && status <= 299) {
    GeminiJSONParser::GetTickerPriceFromJSON(body, &price);
  }
  std::move(callback).Run(price);
}

bool GeminiService::GetAccountBalances(GetAccountBalancesCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnGetAccountBalances,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host_, api_path_account_balances);
  return OAuthRequest(
      url, "POST", "", std::move(internal_callback), true, true, "");
}

void GeminiService::OnGetAccountBalances(
  GetAccountBalancesCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  GeminiAccountBalances balances;
  bool auth_invalid = status == 401;
  if (status >= 200 && status <= 299) {
    const std::string json_body = "{\"data\": " + body + "}";
    GeminiJSONParser::GetAccountBalancesFromJSON(json_body, &balances);
  }
  std::move(callback).Run(balances, auth_invalid);
}

bool GeminiService::GetDepositInfo(const std::string& asset,
                                   GetDepositInfoCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnGetDepositInfo,
      base::Unretained(this), std::move(callback));
  std::string endpoint =
    std::string(api_path_account_addresses) + "/" + asset;
  std::string payload = GetEncodedRequestPayload(endpoint);
  GURL url = GetURLWithPath(api_host_, endpoint);
  return OAuthRequest(
      url, "POST", "", std::move(internal_callback), true, true, payload);
}

void GeminiService::OnGetDepositInfo(
    GetDepositInfoCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string deposit_address;
  if (status >= 200 && status <= 299) {
    const std::string json_body = "{\"data\": " + body + "}";
    GeminiJSONParser::GetDepositInfoFromJSON(json_body, &deposit_address);
  }
  std::move(callback).Run(deposit_address);
}

bool GeminiService::RevokeAccessToken(RevokeAccessTokenCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnRevokeAccessToken,
      base::Unretained(this), std::move(callback));
  std::string payload = GetEncodedRequestPayload(api_path_revoke_token);
  GURL url = GetURLWithPath(api_host_, api_path_revoke_token);
  return OAuthRequest(
      url, "POST", "", std::move(internal_callback), true, true, payload);
}

void GeminiService::OnRevokeAccessToken(
    RevokeAccessTokenCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  bool success = status >= 200 && status <= 299;
  if (success) {
    code_challenge_ = "";
    code_verifier_ = "";
    ResetAccessTokens();
  }
  std::move(callback).Run(success);
}

bool GeminiService::GetOrderQuote(const std::string& side,
                                  const std::string& symbol,
                                  const std::string& spend,
                                  GetOrderQuoteCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnGetOrderQuote,
      base::Unretained(this), std::move(callback), side);
  std::string endpoint =
    std::string(api_path_get_quote) + "/" + side + "/" + symbol;
  std::string payload = GetEncodedRequestPayload(endpoint);
  GURL url = GetURLWithPath(api_host_, endpoint);
  url = net::AppendQueryParameter(url, "totalSpend", spend);
  return OAuthRequest(
      url, "GET", "", std::move(internal_callback), true, true, payload);
}

void GeminiService::OnGetOrderQuote(GetOrderQuoteCallback callback,
    const std::string& side, const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string fee;
  std::string quote_id;
  std::string quantity;
  std::string price;
  std::string error;
  std::string total_price;
  if (status >= 200 && status <= 299) {
    const std::string json_body = "{\"data\": " + body + "}";
    GeminiJSONParser::GetOrderQuoteInfoFromJSON(
      json_body, &quote_id, &quantity,
      &fee, &price, &total_price, &error);
  }
  if (side == "sell") {
    CalculateSaleAmount(quantity, price, fee, &total_price);
  }
  std::move(callback).Run(
    quote_id, quantity, fee, price, total_price, error);
}

bool GeminiService::ExecuteOrder(const std::string& symbol,
                                 const std::string& side,
                                 const std::string& quantity,
                                 const std::string& price,
                                 const std::string& fee,
                                 const int quote_id,
                                 ExecuteOrderCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnOrderExecuted,
      base::Unretained(this), std::move(callback));
  std::string payload = GetEncodedExecutePayload(
    symbol, side, quantity, price, fee, quote_id);
  GURL url = GetURLWithPath(api_host_, api_path_execute_quote);
  return OAuthRequest(
      url, "POST", "", std::move(internal_callback), true, true, payload);
}

void GeminiService::OnOrderExecuted(ExecuteOrderCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  bool success = (status >= 200 && status <= 299);
  std::move(callback).Run(success);
}

bool GeminiService::SetAccessTokens(const std::string& access_token,
                                    const std::string& refresh_token) {
  access_token_ = access_token;
  refresh_token_ = refresh_token;

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;

  if (!OSCrypt::EncryptString(access_token, &encrypted_access_token)) {
    LOG(ERROR) << "Could not encrypt and save Gemini access token";
    return false;
  }
  if (!OSCrypt::EncryptString(refresh_token, &encrypted_refresh_token)) {
    LOG(ERROR) << "Could not encrypt and save Gemini access token";
    return false;
  }

  std::string encoded_encrypted_access_token;
  std::string encoded_encrypted_refresh_token;
  base::Base64Encode(encrypted_access_token, &encoded_encrypted_access_token);
  base::Base64Encode(
      encrypted_refresh_token, &encoded_encrypted_refresh_token);

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  prefs->SetString(kGeminiAccessToken, encoded_encrypted_access_token);
  prefs->SetString(kGeminiRefreshToken, encoded_encrypted_refresh_token);

  return true;
}

bool GeminiService::LoadTokensFromPrefs() {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  std::string encoded_encrypted_access_token =
      prefs->GetString(kGeminiAccessToken);
  std::string encoded_encrypted_refresh_token =
      prefs->GetString(kGeminiRefreshToken);

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;
  if (!base::Base64Decode(encoded_encrypted_access_token,
                          &encrypted_access_token) ||
      !base::Base64Decode(encoded_encrypted_refresh_token,
                          &encrypted_refresh_token)) {
    LOG(ERROR) << "Could not decode Gemini Token info";
    return false;
  }

  if (!OSCrypt::DecryptString(encrypted_access_token, &access_token_)) {
    LOG(ERROR) << "Could not decrypt and save Gemini access token";
    return false;
  }
  if (!OSCrypt::DecryptString(encrypted_refresh_token, &refresh_token_)) {
    LOG(ERROR) << "Could not decrypt and save Gemini refresh token";
    return false;
  }

  return true;
}

void GeminiService::ResetAccessTokens() {
  access_token_ = "";
  refresh_token_ = "";

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  prefs->SetString(kGeminiAccessToken, access_token_);
  prefs->SetString(kGeminiRefreshToken, refresh_token_);
}

bool GeminiService::OAuthRequest(const GURL &url,
                                  const std::string& method,
                                  const std::string& post_data,
                                  URLRequestCallback callback,
                                  bool auto_retry_on_network_change,
                                  bool set_auth_header,
                                  const std::string& payload) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SAVE_COOKIES;
  request->method = method;

  if (set_auth_header) {
    request->headers.SetHeader(
          net::HttpRequestHeaders::kAuthorization,
          base::StringPrintf("Bearer %s", access_token_.c_str()));
  }

  if (!payload.empty()) {
    request->headers.SetHeader("X-GEMINI-PAYLOAD", payload);
  }

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  if (!post_data.empty()) {
    url_loader->AttachStringForUpload(post_data,
        "application/json");
  }
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      auto_retry_on_network_change ?
          network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE :
          network::SimpleURLLoader::RetryMode::RETRY_NEVER);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  auto* default_storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(context_);
  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory,
      base::BindOnce(&GeminiService::OnURLLoaderComplete,
                     base::Unretained(this), iter, std::move(callback)));

  return true;
}

void GeminiService::OnURLLoaderComplete(
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
  std::move(callback).Run(
      response_code, response_body ? *response_body : "", headers);
}

base::SequencedTaskRunner* GeminiService::io_task_runner() {
  if (!io_task_runner_) {
    io_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}

void GeminiService::SetClientIdForTest(const std::string& client_id) {
  client_id_ = client_id;
}

void GeminiService::SetClientSecretForTest(const std::string& client_secret) {
  client_secret_ = client_secret;
}

void GeminiService::SetOAuthHostForTest(const std::string& oauth_host) {
  oauth_host_ = oauth_host;
}

void GeminiService::SetApiHostForTest(const std::string& api_host) {
  api_host_ = api_host;
}
