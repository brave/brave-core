/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/token.h"
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_json_parser.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
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

CryptoDotComService::CryptoDotComService(content::BrowserContext* context)
    : context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
}

CryptoDotComService::~CryptoDotComService() {
}

bool CryptoDotComService::GetTickerInfo(const std::string& asset,
                                        GetTickerInfoCallback callback) {
  auto internal_callback = base::BindOnce(&CryptoDotComService::OnTickerInfo,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, get_ticker_info_path);
  url = net::AppendQueryParameter(url, "instrument_name", asset);
  return NetworkRequest(
      url, "GET", "", std::move(internal_callback));
}

void CryptoDotComService::OnTickerInfo(
  GetTickerInfoCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  CryptoDotComTickerInfo info;
  if (status >= 200 && status <= 299) {
    const std::string json_body = GetFormattedResponseBody(body);
    CryptoDotComJSONParser::GetTickerInfoFromJSON(json_body, &info);
  }
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
  return NetworkRequest(
      url, "GET", "", std::move(internal_callback));
}

void CryptoDotComService::OnChartData(
  GetChartDataCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  CryptoDotComChartData data;
  if (status >= 200 && status <= 299) {
    const std::string json_body = GetFormattedResponseBody(body);
    CryptoDotComJSONParser::GetChartDataFromJSON(json_body, &data);
  }
  std::move(callback).Run(data);
}

bool CryptoDotComService::GetSupportedPairs(
    GetSupportedPairsCallback callback) {
  auto internal_callback = base::BindOnce(
      &CryptoDotComService::OnSupportedPairs,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, get_pairs_path);
  return NetworkRequest(
      url, "GET", "", std::move(internal_callback));
}

void CryptoDotComService::OnSupportedPairs(
  GetSupportedPairsCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  CryptoDotComSupportedPairs pairs;
  if (status >= 200 && status <= 299) {
    const std::string json_body = GetFormattedResponseBody(body);
    CryptoDotComJSONParser::GetPairsFromJSON(json_body, &pairs);
  }
  std::move(callback).Run(pairs);
}

bool CryptoDotComService::GetAssetRankings(
    GetAssetRankingsCallback callback) {
  auto internal_callback = base::BindOnce(
      &CryptoDotComService::OnAssetRankings,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(root_host, get_gainers_losers_path);
  return NetworkRequest(
      url, "GET", "", std::move(internal_callback));
}

void CryptoDotComService::OnAssetRankings(
    GetAssetRankingsCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  CryptoDotComAssetRankings rankings;
  if (status >= 200 && status <= 299) {
    const std::string json_body = GetFormattedResponseBody(body);
    CryptoDotComJSONParser::GetRankingsFromJSON(json_body, &rankings);
  }
  std::move(callback).Run(rankings);
}

bool CryptoDotComService::NetworkRequest(const GURL &url,
                                  const std::string& method,
                                  const std::string& post_data,
                                  URLRequestCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE |
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
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  auto* default_storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(context_);
  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory,
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

base::SequencedTaskRunner* CryptoDotComService::io_task_runner() {
  if (!io_task_runner_) {
    io_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}
