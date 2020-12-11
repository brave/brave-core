/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ftx/browser/ftx_service.h"

#include <string>
#include <utility>

#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ftx/browser/ftx_json_parser.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

namespace {

const char api_host[] = "ftx.com";
// const char com_oauth_host[] = "ftx.com";
// const char us_oauth_host[] = "ftx.us";
const unsigned int kRetriesCountOnNetworkChange = 1;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ftx_service", R"(
      semantics {
        sender: "FTX Service"
        description:
          "This service is used to communicate with FTX "
          "on behalf of the user interacting with the FTX widget."
        trigger:
          "Triggered by user connecting the FTX widget."
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

}  // namespace

FTXService::FTXService(content::BrowserContext* context)
    : context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
}

FTXService::~FTXService() {
}

bool FTXService::GetFuturesData(GetFuturesDataCallback callback) {
  auto internal_callback = base::BindOnce(&FTXService::OnFuturesData,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host, get_futures_data_path);
  return NetworkRequest(
      url, "GET", "", std::move(internal_callback));  
}

void FTXService::OnFuturesData(
  GetFuturesDataCallback callback,
  const int status, const std::string& body,
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
  auto internal_callback = base::BindOnce(&FTXService::OnChartData,
      base::Unretained(this), std::move(callback));
  GURL url = GetURLWithPath(api_host,
      std::string(get_market_data_path) + "/" + symbol + "/candles");
  url = net::AppendQueryParameter(url, "resolution", "14400");
  url = net::AppendQueryParameter(url, "limit", "42");
  url = net::AppendQueryParameter(url, "start_time", start);
  url = net::AppendQueryParameter(url, "end_time", end);
  return NetworkRequest(
      url, "GET", "", std::move(internal_callback));  
}

void FTXService::OnChartData(
  GetChartDataCallback callback,
  const int status, const std::string& body,
  const std::map<std::string, std::string>& headers) {
  FTXChartData data;
  if (status >= 200 && status <= 299) {
    FTXJSONParser::GetChartDataFromJSON(body, &data);
  }
  std::move(callback).Run(data);
}

bool FTXService::NetworkRequest(const GURL &url,
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
      url_loader_factory, base::BindOnce(
          &FTXService::OnURLLoaderComplete,
          base::Unretained(this), std::move(iter), std::move(callback)));

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

  std::move(callback).Run(
      response_code, response_body ? *response_body : "", headers);
}

base::SequencedTaskRunner* FTXService::io_task_runner() {
  if (!io_task_runner_) {
    io_task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}
