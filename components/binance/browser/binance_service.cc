/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/binance/browser/binance_service.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/token.h"
#include "brave/common/pref_names.h"
#include "brave/components/binance/browser/binance_json_parser.h"
#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

GURL BinanceService::oauth_endpoint_("https://accounts.binance.com");
GURL BinanceService::api_endpoint_("https://api.binance.com");

namespace {

// To do, Add as env var
const std::string client_secret = "";
const std::string client_id = "Fx5sIiTEI5";
const std::string oauth_url =
    "https://accounts.binance.com/en/oauth/authorize";
const std::string encoded_uri =
    "https%3A%2F%2Fwww.brave.com%2Fbinance%2Foauth%2Fcallback";
const std::string oauth_scope = "user:email,user:address";

const unsigned int kRetriesCountOnNetworkChange = 1;
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("binance_service", R"(
      semantics {
        sender: "Binance Service"
        description:
          "This service is used to communicate with Binance "
          "on behalf of the user interacting with the Binance widget."
        trigger:
          "Triggered by user connecting the Binance widget."
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

}  // namespace

BinanceService::BinanceService(content::BrowserContext* context)
    : context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
  LoadTokensFromPrefs();
}

BinanceService::~BinanceService() {
}

std::string BinanceService::GetOAuthClientUrl () {
  // To do, use a better formatting solution :)
  const std::string client_url =
      oauth_url + "?response_type=code&client_id=" + client_id +
      "&redirect_uri=" + encoded_uri + "&scope=" + oauth_scope +
      "&code_challenge=" + code_challenge_ + "&code_challenge_method=S256";
  return client_url;
}

bool BinanceService::GetAccessToken(
    const std::string& code,
    GetAccessTokenCallback callback) {
  auto internal_callback = base::BindOnce(
      &BinanceService::OnGetAccessToken,
      base::Unretained(this), std::move(callback));

  const std::string code_param = std::string("code=") + code;
  const std::string grant_type = std::string("grant_type=authorization_code");
  const std::string id_param = std::string("client_id=") + client_id;
  const std::string secret_param = std::string("client_secret=") + client_secret;
  const std::string redirect_uri = std::string("redirect_uri=") + encoded_uri;
  // To do, use a better formatting solution :)
  const std::string formatted_params =
      "?code=" + code_param + "&grant_type=authorization_code&client_id=" +
      id_param + "&client_secret=" + secret_param + "&redirect_uri=" +
      redirect_uri;

  return OAuthRequest(false, oauth_path_access_token,
                     formatted_params,
                     std::move(internal_callback));
}

bool BinanceService::GetAccountBalances(
    GetAccountBalancesCallback callback) {
  auto internal_callback = base::BindOnce(
       &BinanceService::OnGetAccountBalances,
       base::Unretained(this), std::move(callback));
  
  return OAuthRequest(false, oauth_path_account_balances,
                      std::string("?access_token=") + access_token_,
                      std::move(internal_callback));
}

void BinanceService::OnGetAccountBalances(
    GetAccountBalancesCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::map<std::string, std::string> balances;

  if (status >= 200 && status <= 299) {
    BinanceJSONParser::GetAccountBalancesFromJSON(body, &balances);
  }

  std::move(callback).Run(balances, IsUnauthorized(status));
}

void BinanceService::SetCodeChallenge(
    const std::string& challenge,
    SetCodeChallengeCallback callback) {
  bool success = SetCodeChallengePref(challenge);
  std::move(callback).Run(success);
}

void BinanceService::OnGetAccessToken(
    GetAccessTokenCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string access_token;
  std::string refresh_token;

  if (status >= 200 && status <= 299) {
    BinanceJSONParser::GetTokensFromJSON(body, &access_token, "access_token");
    BinanceJSONParser::GetTokensFromJSON(body, &refresh_token, "refresh_token");
    SetAccessTokens(access_token, refresh_token);
  }

  std::move(callback).Run(IsUnauthorized(status));
}

bool BinanceService::OAuthRequest(bool use_version_one,
                                     const std::string& path,
                                     const std::string& query_params,
                                     URLRequestCallback callback) {
  std::string base_url = use_version_one ?
      api_endpoint_.spec() : oauth_endpoint_.spec();             
  std::string request_url = base_url + path + query_params;
  auto request = std::make_unique<network::ResourceRequest>();

  request->url = GURL(request_url);
  request->load_flags = net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE;
  request->method = "POST";

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  Profile* profile = Profile::FromBrowserContext(context_);
  auto* default_storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(profile);
  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory, base::BindOnce(
          &BinanceService::OnURLLoaderComplete,
          base::Unretained(this), std::move(iter), std::move(callback)));

  return true;
}

void BinanceService::OnURLLoaderComplete(
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

bool BinanceService::IsUnauthorized(int status) {
  return status == 401;
}

bool BinanceService::SetAccessTokens(const std::string& access_token,
                                        const std::string& refresh_token) {
  access_token_ = access_token;
  refresh_token_ = refresh_token;

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;

  if (!OSCrypt::EncryptString(access_token, &encrypted_access_token)) {
    LOG(ERROR) << "Could not encrypt and save Binance token info";
    return false;
  }
  if (!OSCrypt::EncryptString(refresh_token, &encrypted_refresh_token)) {
    LOG(ERROR) << "Could not encrypt and save Binance token info";
    return false;
  }

  std::string encoded_encrypted_access_token;
  std::string encoded_encrypted_refresh_token;
  base::Base64Encode(encrypted_access_token, &encoded_encrypted_access_token);
  base::Base64Encode(
      encrypted_refresh_token, &encoded_encrypted_refresh_token);

  Profile* profile = Profile::FromBrowserContext(context_);
  profile->GetPrefs()->SetString(
      kBinanceAccessToken, encoded_encrypted_access_token);
  profile->GetPrefs()->SetString(kBinanceRefreshToken,
      encoded_encrypted_refresh_token);

  return true;
}

bool BinanceService::SetCodeChallengePref(const std::string& challenge) {
  code_challenge_ = challenge;

  std::string encrypted_code_challenge;

  if (!OSCrypt::EncryptString(challenge, &encrypted_code_challenge)) {
    LOG(ERROR) << "Could not encrypt and save Binance code challenge";
    return false;
  }

  std::string encoded_encrypted_code_challenge;
  base::Base64Encode(encrypted_code_challenge,
       &encoded_encrypted_code_challenge);

  Profile* profile = Profile::FromBrowserContext(context_);
  profile->GetPrefs()->SetString(
      kBinanceCodeChallenge, challenge);

  return true;
}

bool BinanceService::LoadTokensFromPrefs() {
  Profile* profile = Profile::FromBrowserContext(context_);
  std::string encoded_encrypted_access_token =
      profile->GetPrefs()->GetString(kBinanceAccessToken);
  std::string encoded_encrypted_refresh_token =
      profile->GetPrefs()->GetString(kBinanceRefreshToken);

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;
  if (!base::Base64Decode(encoded_encrypted_access_token, &encrypted_access_token) ||
      !base::Base64Decode(encoded_encrypted_refresh_token,
                          &encrypted_refresh_token)) {
    LOG(ERROR) << "Could not Base64 decode Binance token info.";
    return false;
  }

  if (!OSCrypt::DecryptString(encrypted_access_token, &access_token_)) {
    LOG(ERROR) << "Could not decrypt and save Binance token info.";
    return false;
  }
  if (!OSCrypt::DecryptString(encrypted_refresh_token, &refresh_token_)) {
    LOG(ERROR) << "Could not decrypt and save Binance token info.";
    return false;
  }

  return true;
}

std::string BinanceService::GetBinanceTLD() {
  Profile* profile = Profile::FromBrowserContext(context_);

  const std::string usTLD = "us";
  const std::string usCode = "US";
  const std::string globalTLD = "com";

  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());
  const int32_t us_id = country_codes::CountryCharsToCountryID(
      usCode.at(0), usCode.at(1));

  return (user_country_id == us_id) ? usTLD : globalTLD;
}

bool BinanceService::GetConvertQuote(
    const std::string& from,
    const std::string& to,
    const std::string& amount,
    GetConvertQuoteCallback callback) {
  auto internal_callback = base::BindOnce(
      &BinanceService::OnGetConvertQuote,
      base::Unretained(this), std::move(callback));

  const std::string from_param = std::string("&fromAsset=" + from);
  const std::string to_param = std::string("&toAsset=" + to);
  const std::string base_param = std::string("&baseAsset=" + to);
  const std::string amount_param = std::string("&amount=" + amount);
  const std::string access_param = std::string("?access_token=") + access_token_;

  const std::string formatted_params =
      access_param + amount_param + base_param +
      to_param + from_param;

  return OAuthRequest(false, oauth_path_convert_quote,
                     formatted_params,
                     std::move(internal_callback));
}

void BinanceService::OnGetConvertQuote(
    GetConvertQuoteCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string quote_id;

  if (status >= 200 && status <= 299) {
    BinanceJSONParser::GetQuoteIDFromJSON(body, &quote_id);
  }

  std::move(callback).Run(quote_id);
}

bool BinanceService::GetTickerPrice(
    const std::string& symbol_pair,
    GetTickerPriceCallback callback) {
  auto internal_callback = base::BindOnce(
      &BinanceService::OnGetTickerPrice,
      base::Unretained(this), std::move(callback));
  const std::string formatted_params =
        std::string("?symbol=") + symbol_pair;
  return OAuthRequest(true, api_path_ticker_price,
                     formatted_params,
                     std::move(internal_callback));
}

bool BinanceService::GetTickerVolume(
    const std::string& symbol_pair,
    GetTickerVolumeCallback callback) {
  auto internal_callback = base::BindOnce(
      &BinanceService::OnGetTickerVolume,
      base::Unretained(this), std::move(callback));
  const std::string formatted_params =
        std::string("?symbol=") + symbol_pair;
  return OAuthRequest(true, api_path_ticker_volume,
                    formatted_params,
                    std::move(internal_callback));
}

void BinanceService::OnGetTickerPrice(
    GetTickerPriceCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string symbol_pair_price = "0.00";
  if (status >= 200 && status <= 299) {
    BinanceJSONParser::GetTickerPriceFromJSON(body, &symbol_pair_price);
  }
  std::move(callback).Run(symbol_pair_price);
}

void BinanceService::OnGetTickerVolume(
    GetTickerPriceCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string symbol_pair_volume = "0";
  if (status >= 200 && status <= 299) {
    BinanceJSONParser::GetTickerVolumeFromJSON(body, &symbol_pair_volume);
  }
  std::move(callback).Run(symbol_pair_volume);
}

base::SequencedTaskRunner* BinanceService::io_task_runner() {
  if (!io_task_runner_) {
    io_task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}
