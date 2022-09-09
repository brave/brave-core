/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"

#include <utility>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "url/gurl.h"

namespace {

constexpr int kMaxBodySize = 1024 * 1024;

const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("wayback_machine_infobar", R"(
        semantics {
          sender:
            "Brave Wayback Machine"
          description:
            "Download wayback url"
          trigger:
            "When user gets 404 page"
          data: "current tab's url"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          policy_exception_justification:
            "Not implemented."
        })");
  return network_traffic_annotation_tag;
}

}  // namespace

WaybackMachineURLFetcher::WaybackMachineURLFetcher(
    Client* client,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : client_(client),
      api_request_helper_(new api_request_helper::APIRequestHelper(
          GetNetworkTrafficAnnotationTag(),
          url_loader_factory)) {}

WaybackMachineURLFetcher::~WaybackMachineURLFetcher() = default;

void WaybackMachineURLFetcher::Fetch(const GURL& url) {
  const GURL wayback_fetch_url(std::string(kWaybackQueryURL) + url.spec());
  api_request_helper_->Request(
      "GET", FixupWaybackQueryURL(wayback_fetch_url), std::string(),
      "application/json", true,
      base::BindOnce(&WaybackMachineURLFetcher::OnWaybackURLFetched,
                     base::Unretained(this), url),
      {}, kMaxBodySize);
}

void WaybackMachineURLFetcher::OnWaybackURLFetched(
    const GURL& original_url,
    api_request_helper::APIRequestResult api_request_result) {
  const auto& response_body = api_request_result.body();
  if (response_body.empty()) {
    client_->OnWaybackURLFetched(GURL::EmptyGURL());
    return;
  }

  const auto result = base::JSONReader::Read(response_body);
  if (!result || !result->FindPath("archived_snapshots.closest.url")) {
    client_->OnWaybackURLFetched(GURL::EmptyGURL());
    return;
  }

  client_->OnWaybackURLFetched(
      GURL(result->FindPath("archived_snapshots.closest.url")->GetString()));
}
