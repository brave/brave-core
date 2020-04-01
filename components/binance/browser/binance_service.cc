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
#include "base/strings/string_number_conversions.h"
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
#include "crypto/random.h"
#include "crypto/sha2.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

const char client_id[] = BINANCE_CLIENT_ID;
const char oauth_host[] = "accounts.binance.com";
const char api_host[] = "api.binance.com";
const char oauth_callback[] = "com.brave.binance://authorization";
const char oauth_scope[] = "user:email,user:address,asset:balance";
const GURL oauth_url("https://accounts.binance.com/en/oauth/authorize");
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

GURL GetURLWithPath(const std::string& host, const std::string& path) {
  return GURL(std::string(url::kHttpsScheme) + "://" + host).Resolve(path);
}

std::string GetHexEncodedCryptoRandomSeed() {
  const size_t kSeedByteLength = 28;
  // crypto::RandBytes is fail safe.
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes, kSeedByteLength);
  return base::HexEncode(
      reinterpret_cast<char*>(random_seed_bytes), kSeedByteLength);
}

std::string GetCodeChallenge(const std::string& code_verifier) {
  std::string code_challenge;
  char raw[crypto::kSHA256Length] = {0};
  crypto::SHA256HashString(code_verifier,
                           raw,
                           crypto::kSHA256Length);
  base::Base64Encode(base::StringPiece(raw,
                                       crypto::kSHA256Length),
                                       &code_challenge);

  // Binance expects the following conversions for the base64 encoded value:
  std::replace(code_challenge.begin(), code_challenge.end(), '+', '-');
  std::replace(code_challenge.begin(), code_challenge.end(), '/', '_');
  code_challenge.erase(std::find_if(code_challenge.rbegin(),
      code_challenge.rend(), [](int ch) {
    return ch != '=';
  }).base(), code_challenge.end());

  return code_challenge;
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

std::string BinanceService::GetOAuthClientUrl() {
  // The code_challenge_ value is derived from the code_verifier value.
  // Step 1 of the oauth process uses the code_challenge_ value.
  // Step 4 of the oauth process uess the code_verifer_.
  // We never need to persist these values, they are just used to get an
  // access token.
  code_verifier_ = GetHexEncodedCryptoRandomSeed();
  code_challenge_ = GetCodeChallenge(code_verifier_);

  GURL url(oauth_url);
  url = net::AppendQueryParameter(url, "response_type", "code");
  url = net::AppendQueryParameter(url, "client_id", client_id);
  url = net::AppendQueryParameter(url, "redirect_uri", oauth_callback);
  url = net::AppendQueryParameter(url, "scope", oauth_scope);
  url = net::AppendQueryParameter(url, "code_challenge", code_challenge_);
  url = net::AppendQueryParameter(url, "code_challenge_method", "S256");
  return url.spec();
}

bool BinanceService::GetAccessToken(const std::string& code,
                                    GetAccessTokenCallback callback) {
  auto internal_callback = base::BindOnce(&BinanceService::OnGetAccessToken,
      base::Unretained(this), std::move(callback));
  GURL base_url = GetURLWithPath(oauth_host, oauth_path_access_token);
  GURL url = base_url;
  url = net::AppendQueryParameter(url, "grant_type", "authorization_code");
  url = net::AppendQueryParameter(url, "code", code);
  url = net::AppendQueryParameter(url, "client_id", client_id);
  url = net::AppendQueryParameter(url, "code_verifier", code_verifier_);
  url = net::AppendQueryParameter(url, "redirect_uri", oauth_callback);
  return OAuthRequest(base_url, "POST", url.query(), std::move(internal_callback));
}

bool BinanceService::GetAccountBalances(GetAccountBalancesCallback callback) {
  auto internal_callback = base::BindOnce(&BinanceService::OnGetAccountBalances,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(oauth_host, oauth_path_account_balances);
  url = net::AppendQueryParameter(url, "access_token", access_token_);
  return OAuthRequest(url, "GET", "", std::move(internal_callback));
}

void BinanceService::OnGetAccountBalances(GetAccountBalancesCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::map<std::string, std::string> balances;

  if (status >= 200 && status <= 299) {
    BinanceJSONParser::GetAccountBalancesFromJSON(body, &balances);
  }

  std::move(callback).Run(balances, !IsUnauthorized(status));
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

  std::move(callback).Run(!IsUnauthorized(status));
}

bool BinanceService::OAuthRequest(const GURL &url,
                                  const std::string& method,
                                  const std::string& post_data,
                                  URLRequestCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE;
  request->method = method;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  if (!post_data.empty()) {
    url_loader->AttachStringForUpload(post_data,
        "application/x-www-form-urlencoded");
  }
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

bool BinanceService::LoadTokensFromPrefs() {
  Profile* profile = Profile::FromBrowserContext(context_);
  std::string encoded_encrypted_access_token =
      profile->GetPrefs()->GetString(kBinanceAccessToken);
  std::string encoded_encrypted_refresh_token =
      profile->GetPrefs()->GetString(kBinanceRefreshToken);

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;
  if (!base::Base64Decode(encoded_encrypted_access_token,
                          &encrypted_access_token) ||
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
  auto internal_callback = base::BindOnce(&BinanceService::OnGetConvertQuote,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(oauth_host, oauth_path_convert_quote);
  url = net::AppendQueryParameter(url, "fromAsset", from);
  url = net::AppendQueryParameter(url, "to", to);
  url = net::AppendQueryParameter(url, "baseAsset", to);
  url = net::AppendQueryParameter(url, "amount", amount);
  url = net::AppendQueryParameter(url, "access_token", access_token_);
  return OAuthRequest(url, "POST", "", std::move(internal_callback));
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
  auto internal_callback = base::BindOnce(&BinanceService::OnGetTickerPrice,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, api_path_ticker_price);
  url = net::AppendQueryParameter(url, "symbol", symbol_pair);
  return OAuthRequest(url, "POST", "", std::move(internal_callback));
}

bool BinanceService::GetTickerVolume(
    const std::string& symbol_pair,
    GetTickerVolumeCallback callback) {
  auto internal_callback = base::BindOnce(&BinanceService::OnGetTickerVolume,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, api_path_ticker_volume);
  url = net::AppendQueryParameter(url, "symbol", symbol_pair);
  return OAuthRequest(url, "POST", "", std::move(internal_callback));
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
