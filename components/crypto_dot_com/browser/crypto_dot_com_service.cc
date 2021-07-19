/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_json_parser.h"
#include "brave/components/crypto_dot_com/common/constants.h"
#include "brave/components/crypto_dot_com/common/pref_names.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

namespace {

const char root_host[] = "crypto.com";
const char api_host[] = "api.crypto.com";
const unsigned int kRetriesCountOnNetworkChange = 1;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("crypto_dot_com_service", R"(
      semantics {
        sender: "CryptoDotCom Service"
        description:
          "This service is used to communicate with CryptoDotCom "
          "on behalf of the user interacting with the CryptoDotCom widget."
        trigger:
          "Triggered by user connecting the CryptoDotCom widget."
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

std::string GetFormattedResponseBody(const std::string& json_response) {
  return "{\"response\": " + json_response + "}";
}

}  // namespace

CryptoDotComService::CryptoDotComService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {
  LoadTokenFromPrefs();
}

CryptoDotComService::~CryptoDotComService() = default;

bool CryptoDotComService::GetTickerInfo(const std::string& asset,
                                        GetTickerInfoCallback callback) {
  auto internal_callback = base::BindOnce(&CryptoDotComService::OnTickerInfo,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, get_ticker_info_path);
  url = net::AppendQueryParameter(url, "instrument_name", asset);
  return NetworkRequest(url, "GET", "", net::HttpRequestHeaders(),
                        std::move(internal_callback));
}

void CryptoDotComService::OnTickerInfo(
  GetTickerInfoCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  CryptoDotComTickerInfo info;
  const std::string json_body = GetFormattedResponseBody(body);
  // If |json_body| is invalid, empty |info| is passed.
  CryptoDotComJSONParser::GetTickerInfoFromJSON(json_body, &info);
  std::move(callback).Run(info);
}

bool CryptoDotComService::GetChartData(const std::string& asset,
                                       GetChartDataCallback callback) {
  auto internal_callback = base::BindOnce(&CryptoDotComService::OnChartData,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, get_chart_data_path);
  url = net::AppendQueryParameter(url, "instrument_name", asset);
  url = net::AppendQueryParameter(url, "timeframe", "4h");
  url = net::AppendQueryParameter(url, "depth", "42");
  return NetworkRequest(url, "GET", "", net::HttpRequestHeaders(),
                        std::move(internal_callback));
}

void CryptoDotComService::OnChartData(
  GetChartDataCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  CryptoDotComChartData data;
  const std::string json_body = GetFormattedResponseBody(body);
  if (!CryptoDotComJSONParser::GetChartDataFromJSON(json_body, &data)) {
    const std::map<std::string, double> empty_data_point = {
        {"t", 0}, {"o", 0}, {"h", 0}, {"l", 0}, {"c", 0}, {"v", 0}};
    data.push_back(empty_data_point);
  }
  std::move(callback).Run(data);
}

bool CryptoDotComService::GetSupportedPairs(
    GetSupportedPairsCallback callback) {
  auto internal_callback = base::BindOnce(
      &CryptoDotComService::OnSupportedPairs,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, get_pairs_path);
  return NetworkRequest(url, "GET", "", net::HttpRequestHeaders(),
                        std::move(internal_callback));
}

void CryptoDotComService::OnSupportedPairs(
  GetSupportedPairsCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  CryptoDotComSupportedPairs pairs;
  const std::string json_body = GetFormattedResponseBody(body);
  if (!CryptoDotComJSONParser::GetPairsFromJSON(json_body, &pairs)) {
    const std::map<std::string, std::string> empty_pair = {{"pair", ""},
                                                           {"quote", ""},
                                                           {"base", ""},
                                                           {"price", ""},
                                                           {"quantity", ""}};
    pairs.push_back(empty_pair);
  }
  std::move(callback).Run(pairs);
}

bool CryptoDotComService::GetAssetRankings(
    GetAssetRankingsCallback callback) {
  auto internal_callback = base::BindOnce(
      &CryptoDotComService::OnAssetRankings,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(root_host, get_gainers_losers_path);
  std::map<std::string, std::string> headers;
  return NetworkRequest(url, "GET", "", net::HttpRequestHeaders(),
                        std::move(internal_callback));
}

void CryptoDotComService::OnAssetRankings(
    GetAssetRankingsCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  CryptoDotComAssetRankings rankings;
  const std::string json_body = GetFormattedResponseBody(body);
  if (!CryptoDotComJSONParser::GetRankingsFromJSON(json_body, &rankings)) {
    std::vector<std::map<std::string, std::string>> gainers;
    std::vector<std::map<std::string, std::string>> losers;
    rankings.insert({"gainers", gainers});
    rankings.insert({"losers", losers});
  }
  std::move(callback).Run(rankings);
}

bool CryptoDotComService::GetAccountBalances(
    GetAccountBalancesCallback callback) {
  auto internal_callback =
      base::BindOnce(&CryptoDotComService::OnGetAccountBalances,
                     base::Unretained(this), std::move(callback));
  net::HttpRequestHeaders headers;
  headers.SetHeader("widget-token", access_token_);
  return NetworkRequest(GURL(kCryptoDotComGetAccountBalanceURL), "GET", "",
                        headers, std::move(internal_callback));
}

void CryptoDotComService::OnGetAccountBalances(
    GetAccountBalancesCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  auto value = CryptoDotComJSONParser::GetValidAccountBalances(body);
  if (value.is_none()) {
    auto empty_balance = base::JSONReader::Read(kEmptyAccountBalances);
    return std::move(callback).Run(std::move(*empty_balance));
  }

  std::move(callback).Run(std::move(value));
}

bool CryptoDotComService::IsLoggedIn() {
  return !access_token_.empty();
}

bool CryptoDotComService::Disconnect(DisconnectCallback callback) {
  auto internal_callback =
      base::BindOnce(&CryptoDotComService::OnDisconnect, base::Unretained(this),
                     std::move(callback));
  net::HttpRequestHeaders headers;
  headers.SetHeader("widget-token", access_token_);
  return NetworkRequest(GURL(kCryptoDotComDisconnectURL), "POST", "", headers,
                        std::move(internal_callback));
}

void CryptoDotComService::OnDisconnect(
    DisconnectCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  auto disconnect_value = base::JSONReader::Read(body);
  if (!disconnect_value || !disconnect_value->is_dict()) {
    std::move(callback).Run(false);
    return;
  }

  if (const auto* code = disconnect_value->FindStringKey("code")) {
    if (*code != "0")
      std::move(callback).Run(false);
  }

  std::move(callback).Run(true);
  // Clear if got success
  SetAccessToken("");
}

bool CryptoDotComService::IsConnected(IsConnectedCallback callback) {
  if (access_token_.empty()) {
    std::move(callback).Run(false);
    return true;
  }

  auto internal_callback =
      base::BindOnce(&CryptoDotComService::OnIsConnected,
                     base::Unretained(this), std::move(callback));
  net::HttpRequestHeaders headers;
  headers.SetHeader("widget-token", access_token_);
  return NetworkRequest(GURL(kCryptoDotComGetAccountBalanceURL), "GET", "",
                        headers, std::move(internal_callback));
}

void CryptoDotComService::OnIsConnected(
    IsConnectedCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  auto balances_value = base::JSONReader::Read(body);
  if (!balances_value || !balances_value->is_dict()) {
    std::move(callback).Run(false);
    return;
  }

  // If access token is not valid anymore, returned body looks like
  // {"code":"10002","msg":"Not logged-in","data":null}.
  if (const auto* code = balances_value->FindStringKey("code")) {
    if (*code != "0")
      return std::move(callback).Run(false);
  }

  std::move(callback).Run(true);
}

bool CryptoDotComService::GetDepositAddress(
    const std::string& asset,
    GetDepositAddressCallback callback) {
  auto internal_callback =
      base::BindOnce(&CryptoDotComService::OnGetDepositAddress,
                     base::Unretained(this), std::move(callback), asset);
  net::HttpRequestHeaders headers;
  headers.SetHeader("widget-token", access_token_);
  GURL url(kCryptoDotComGetDepositAddressURL);
  url = net::AppendQueryParameter(url, "currency", asset);
  return NetworkRequest(url, "GET", "", headers, std::move(internal_callback));
}

void CryptoDotComService::OnGetDepositAddress(
    GetDepositAddressCallback callback,
    const std::string& asset,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  auto value = CryptoDotComJSONParser::GetValidDepositAddress(body);
  if (value.is_none()) {
    auto empty_deposit = base::JSONReader::Read(kEmptyDepositAddress);
    return std::move(callback).Run(std::move(*empty_deposit));
  }

  std::move(callback).Run(std::move(value));
}

bool CryptoDotComService::GetNewsEvents(GetNewsEventsCallback callback) {
  auto internal_callback =
      base::BindOnce(&CryptoDotComService::OnGetNewsEvents,
                     base::Unretained(this), std::move(callback));
  net::HttpRequestHeaders headers;
  headers.SetHeader("widget-token", access_token_);
  GURL url(kCryptoDotComGetNewsEventsURL);
  return NetworkRequest(url, "GET", "", headers, std::move(internal_callback));
}

void CryptoDotComService::OnGetNewsEvents(
    GetNewsEventsCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  auto value = CryptoDotComJSONParser::GetValidNewsEvents(body);
  if (value.is_none()) {
    auto empty_news_events = base::JSONReader::Read(kEmptyNewsEvents);
    return std::move(callback).Run(
        empty_news_events->FindListKey("events")->Clone());
  }

  std::move(callback).Run(std::move(value));
}

bool CryptoDotComService::CreateMarketOrder(
    base::Value order,
    CreateMarketOrderCallback callback) {
  auto internal_callback =
      base::BindOnce(&CryptoDotComService::OnCreateMarketOrder,
                     base::Unretained(this), std::move(callback));
  net::HttpRequestHeaders headers;
  headers.SetHeader("widget-token", access_token_);
  GURL url(kCryptoDotComCreateMarketOrderURL);

  std::string body;
  base::JSONWriter::Write(order, &body);
  DVLOG(2) << __func__ << ": " << body;
  return NetworkRequest(url, "POST", body, headers,
                        std::move(internal_callback));
}

void CryptoDotComService::OnCreateMarketOrder(
    CreateMarketOrderCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DVLOG(2) << __func__ << ": " << body;
  std::move(callback).Run(CryptoDotComJSONParser::GetValidOrderResult(body));
}

std::string CryptoDotComService::GetAuthClientUrl() const {
  return kCryptoDotComAuthURL;
}

bool CryptoDotComService::SetAccessToken(const std::string& access_token) {
  access_token_ = access_token;

  if (access_token_.empty()) {
    prefs_->SetString(kCryptoDotComAccessToken, access_token_);
    return true;
  }

  std::string encrypted_access_token;
  if (!OSCrypt::EncryptString(access_token, &encrypted_access_token)) {
    LOG(ERROR) << "Could not encrypt and save crypto.com access token";
    return false;
  }

  std::string encoded_encrypted_access_token;
  base::Base64Encode(encrypted_access_token, &encoded_encrypted_access_token);
  prefs_->SetString(kCryptoDotComAccessToken, encoded_encrypted_access_token);

  return true;
}

bool CryptoDotComService::LoadTokenFromPrefs() {
  std::string encoded_encrypted_access_token =
      prefs_->GetString(kCryptoDotComAccessToken);

  std::string encrypted_access_token;
  if (!base::Base64Decode(encoded_encrypted_access_token,
                          &encrypted_access_token)) {
    LOG(ERROR) << "Could not decode CryptoDotCom Token info";
    return false;
  }

  if (!OSCrypt::DecryptString(encrypted_access_token, &access_token_)) {
    LOG(ERROR) << "Could not decrypt and save crypto.com access token";
    return false;
  }

  return true;
}

bool CryptoDotComService::NetworkRequest(const GURL& url,
                                         const std::string& method,
                                         const std::string& post_data,
                                         const net::HttpRequestHeaders& headers,
                                         URLRequestCallback callback) {
  DVLOG(2) << __func__ << ": " << url;
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SAVE_COOKIES;
  request->method = method;
  request->headers = headers;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());

  if (!post_data.empty()) {
    url_loader->AttachStringForUpload(post_data, "application/json");
  }

  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&CryptoDotComService::OnURLLoaderComplete,
                     base::Unretained(this), iter, std::move(callback)));

  return true;
}

void CryptoDotComService::OnURLLoaderComplete(
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
