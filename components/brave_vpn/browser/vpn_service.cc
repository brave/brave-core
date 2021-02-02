/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/vpn_service.h"

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
#include "brave/components/binance/browser/regions.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_oauth.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"
#include "components/country_codes/country_codes.h"
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

// GURL GetURLWithPath(const std::string& host, const std::string& path) {
//   return GURL(std::string(url::kHttpsScheme) + "://" + host).Resolve(path);
// }

}  // namespace

VpnService::VpnService(content::BrowserContext* context)
    : context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
  // LoadTokensFromPrefs();
}

VpnService::~VpnService() {}

bool VpnService::OAuthRequest(const GURL& url,
                              const std::string& method,
                              const std::string& post_data,
                              URLRequestCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;

  // if (!send_save_cookies) {
  //   request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  //   request->load_flags |= net::LOAD_DO_NOT_SAVE_COOKIES;
  // }

  request->method = method;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  if (!post_data.empty()) {
    url_loader->AttachStringForUpload(post_data, "application/json");
  }
  // url_loader->SetRetryOptions(
  //     kRetriesCountOnNetworkChange,
  //     auto_retry_on_network_change ?
  //         network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE :
  //         network::SimpleURLLoader::RetryMode::RETRY_NEVER);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  auto* default_storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(context_);
  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory,
      base::BindOnce(&VpnService::OnURLLoaderComplete, base::Unretained(this),
                     std::move(iter), std::move(callback)));

  return true;
}

void VpnService::OnURLLoaderComplete(
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

void VpnService::GetAllServerRegions(GetAllServerRegionsCallback callback) {
  auto internal_callback =
      base::BindOnce(&VpnService::OnGetAllServerRegions, base::Unretained(this),
                     std::move(callback));
  GURL base_url = GURL(all_server_regions);
  GURL url = base_url;
  url = net::AppendQueryParameter(url, "region", "en-us");
  LOG(ERROR) << "NTP"
             << "VpnService::GetAllServerRegions" << base_url;
  OAuthRequest(base_url, "GET", url.query(), std::move(internal_callback));
}

void VpnService::OnGetAllServerRegions(
    GetAllServerRegionsCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  ServerRegions regions;

  bool success = status >= 200 && status <= 299;
  LOG(ERROR) << "NTP"
             << "VpnService::OnGetAllServerRegions"
             << "Status code" << status;
  if (success) {
    LOG(ERROR) << "NTP"
               << "VpnService::OnGetAllServerRegions"
               << "Body : " << body;
    // BinanceJSONParser::GetAccountBalancesFromJSON(body, &regions);
  }
  std::move(callback).Run(regions, success);
}
