/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/double_fetcher.h"

#include <utility>

#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/request_queue.h"
#include "brave/components/web_discovery/browser/util.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace web_discovery {

namespace {
constexpr char kUrlKey[] = "url";
constexpr char kAssociatedDataKey[] = "assoc_data";

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
        "Retrieves a page of interest without session cookies for
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
    FetchedCallback callback)
    : profile_prefs_(profile_prefs),
      shared_url_loader_factory_(shared_url_loader_factory),
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

  request_queue_.ScheduleRequest(base::Value(std::move(fetch_dict)));
}

void DoubleFetcher::OnFetchTimer(const base::Value& request_data) {
  const auto* fetch_dict = request_data.GetIfDict();
  const auto* url = fetch_dict ? fetch_dict->FindString(kUrlKey) : nullptr;
  if (!url) {
    request_queue_.NotifyRequestComplete(true);
    return;
  }

  auto resource_request = CreateResourceRequest(GURL(*url));
  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kFetchNetworkTrafficAnnotation);
  url_loader_->DownloadToString(
      shared_url_loader_factory_.get(),
      base::BindOnce(&DoubleFetcher::OnRequestComplete, base::Unretained(this)),
      kMaxDoubleFetchResponseSize);
}

void DoubleFetcher::OnRequestComplete(
    std::optional<std::string> response_body) {
  auto result = ProcessCompletedRequest(&response_body);

  auto request_data = request_queue_.NotifyRequestComplete(result);

  if (request_data) {
    const auto& request_dict = request_data->GetDict();
    const auto* assoc_data = request_dict.Find(kAssociatedDataKey);
    if (assoc_data) {
      callback_.Run(*assoc_data, response_body);
    }
  }
}

bool DoubleFetcher::ProcessCompletedRequest(
    std::optional<std::string>* response_body) {
  auto* response_info = url_loader_->ResponseInfo();
  if (!response_body || !response_info) {
    return false;
  }
  auto response_code = response_info->headers->response_code();
  if (response_code < 200 || response_code >= 300) {
    if (response_code >= 500) {
      // Only retry failures due to server error
      return false;
    }
    *response_body = std::nullopt;
  }
  return true;
}

}  // namespace web_discovery
