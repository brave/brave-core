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
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/token.h"
#include "brave/common/pref_names.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "brave/components/gemini/browser/gemini_json_parser.h"

namespace {
  const char oauth_host[] = "exchange.qa001.aurora7.net";
  // const char api_host[] = "api.qa001.aurora7.net";
  const char oauth_callback[] = "com.brave.gemini://authorization";
  const char oauth_scope[] = "trader";
  const char oauth_url[] = "https://exchange.qa001.aurora7.net/auth";
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

}  // namespace

GeminiService::GeminiService(content::BrowserContext* context)
    : client_id_(GEMINI_CLIENT_ID),
      client_secret_(GEMINI_CLIENT_SECRET),
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
  url = net::AppendQueryParameter(url, "response_type", "code");
  url = net::AppendQueryParameter(url, "client_id", client_id_);
  url = net::AppendQueryParameter(url, "redirect_uri", oauth_callback);
  url = net::AppendQueryParameter(url, "scope", oauth_scope);
  url = net::AppendQueryParameter(url, "state", "ghjk");
  return url.spec();
}

void GeminiService::SetAuthToken(const std::string& auth_token) {
  auth_token_ = auth_token;
}

bool GeminiService::GetAccessToken(GetAccessTokenCallback callback) {
  auto internal_callback = base::BindOnce(&GeminiService::OnGetAccessToken,
      base::Unretained(this), std::move(callback));
  GURL base_url = GetURLWithPath(oauth_host, oauth_path_access_token);
  GURL url = base_url;
  url = net::AppendQueryParameter(url, "client_id", client_id_);
  url = net::AppendQueryParameter(url, "client_secret", client_secret_);
  url = net::AppendQueryParameter(url, "code", auth_token_);
  url = net::AppendQueryParameter(url, "redirect_uri", oauth_callback);
  url = net::AppendQueryParameter(url, "grant_type", "authorization_code");
  auth_token_.clear();
  return OAuthRequest(
      base_url, "POST", url.query(), std::move(internal_callback), true);
}

void GeminiService::OnGetAccessToken(
    GetAccessTokenCallback callback,
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
                                  bool auto_retry_on_network_change) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES;
  request->method = method;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  if (!post_data.empty()) {
    url_loader->AttachStringForUpload(post_data,
        "application/x-www-form-urlencoded");
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
      url_loader_factory, base::BindOnce(
          &GeminiService::OnURLLoaderComplete,
          base::Unretained(this), std::move(iter), std::move(callback)));

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
    io_task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}
