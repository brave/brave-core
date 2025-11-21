/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/double_fetcher.h"

#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_search/browser/backup_results_allowed_urls.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/request_queue.h"
#include "brave/components/web_discovery/browser/util.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace web_discovery {

namespace {
constexpr char kUrlKey[] = "url";
constexpr char kAssociatedDataKey[] = "assoc_data";
constexpr char kSearchPath[] = "/search";

constexpr base::TimeDelta kRequestMaxAge = base::Hours(1);
constexpr base::TimeDelta kMinRequestInterval =
    base::Minutes(1) - base::Seconds(5);
constexpr base::TimeDelta kMaxRequestInterval =
    base::Minutes(1) + base::Seconds(5);
constexpr size_t kMaxRetries = 3;
constexpr size_t kMaxDoubleFetchResponseSize = 2 * 1024 * 1024;

constexpr net::NetworkTrafficAnnotationTag kFetchNetworkTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("wdp_doublefetch", R"(
    semantics {
      sender: "Brave Web Discovery Double Fetch"
      description:
        "Retrieves a page of interest without cookies for
         scraping and reporting via Web Discovery."
      trigger:
        "Requests are sent minutes after the original
         page request is made by the user."
      data: "Page data"
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "Users can opt-in or out via brave://settings/search"
    })");

}  // namespace

DoubleFetcher::DoubleFetcher(
    PrefService* profile_prefs,
    network::SharedURLLoaderFactory* shared_url_loader_factory,
    brave_search::BackupResultsService* backup_results_service,
    FetchedCallback callback)
    : profile_prefs_(profile_prefs),
      shared_url_loader_factory_(shared_url_loader_factory),
      backup_results_service_(backup_results_service),
      request_queue_(profile_prefs,
                     kScheduledDoubleFetches,
                     kRequestMaxAge,
                     kMinRequestInterval,
                     kMaxRequestInterval,
                     kMaxRetries,
                     base::BindRepeating(&DoubleFetcher::OnFetchTimer,
                                         base::Unretained(this))),
      callback_(callback) {}

DoubleFetcher::~DoubleFetcher() = default;

void DoubleFetcher::ScheduleDoubleFetch(const GURL& url,
                                        base::Value associated_data) {
  base::Value::Dict fetch_dict;
  fetch_dict.Set(kUrlKey, url.spec());
  fetch_dict.Set(kAssociatedDataKey, std::move(associated_data));

  request_queue_.ScheduleRequest(std::move(fetch_dict));
}

void DoubleFetcher::OnFetchTimer(const base::Value& request_data) {
  const auto* fetch_dict = request_data.GetIfDict();
  const auto* url_str = fetch_dict ? fetch_dict->FindString(kUrlKey) : nullptr;
  if (!url_str) {
    request_queue_.NotifyRequestComplete(true);
    return;
  }

  GURL url(*url_str);

  if (brave_search::IsBackupResultURLAllowed(url) &&
      base::StartsWith(url.path(), kSearchPath)) {
    CHECK(backup_results_service_);
    backup_results_service_->FetchBackupResults(
        url, {},
        base::BindOnce(&DoubleFetcher::OnRenderedResponse,
                       weak_ptr_factory_.GetWeakPtr(), url));
    return;
  }

  auto resource_request = CreateResourceRequest(url);
  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kFetchNetworkTrafficAnnotation);
  url_loader_->DownloadToString(
      shared_url_loader_factory_.get(),
      base::BindOnce(&DoubleFetcher::OnURLLoaderResponse,
                     base::Unretained(this), url),
      kMaxDoubleFetchResponseSize);
}

void DoubleFetcher::OnURLLoaderResponse(
    const GURL& url,
    std::optional<std::string> response_body) {
  auto* response_info = url_loader_->ResponseInfo();
  std::optional<int> response_code;
  if (response_info) {
    response_code = response_info->headers->response_code();
  }

  OnRequestComplete(url, response_code, std::move(response_body));
}

void DoubleFetcher::OnRenderedResponse(
    const GURL& url,
    std::optional<brave_search::BackupResultsService::BackupResults> results) {
  if (results) {
    OnRequestComplete(url, results->final_status_code, results->html);
  } else {
    OnRequestComplete(url, std::nullopt, std::nullopt);
  }
}

void DoubleFetcher::OnRequestComplete(
    const GURL& url,
    std::optional<int> response_code,
    std::optional<std::string> response_body) {
  bool result = false;
  if (response_code) {
    if (!network::IsSuccessfulStatus(*response_code)) {
      if (response_code >= net::HttpStatusCode::HTTP_BAD_REQUEST &&
          response_code < net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR) {
        // Only retry failures due to server error
        // Mark as 'successful' if not a 5xx error, so we don't retry
        result = true;
      }
      response_body = std::nullopt;
    } else {
      result = true;
    }
  }

  auto request_data = request_queue_.NotifyRequestComplete(result);

  if (request_data) {
    const auto& request_dict = request_data->GetDict();
    const auto* assoc_data = request_dict.Find(kAssociatedDataKey);
    if (assoc_data) {
      callback_.Run(url, *assoc_data, response_body);
    }
  }
}

}  // namespace web_discovery
