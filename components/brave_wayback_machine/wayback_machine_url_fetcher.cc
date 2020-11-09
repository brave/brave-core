/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"

#include <utility>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/base/load_flags.h"
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
      url_loader_factory_(std::move(url_loader_factory)) {
}

WaybackMachineURLFetcher::~WaybackMachineURLFetcher() {
}

void WaybackMachineURLFetcher::Fetch(const GURL& url) {
  auto request = std::make_unique<network::ResourceRequest>();
  std::string wayback_fetch_url =
      std::string(kWaybackQueryURL) + url.spec();
  request->url = GURL(wayback_fetch_url);
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
  wayback_url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  wayback_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&WaybackMachineURLFetcher::OnWaybackURLFetched,
                     base::Unretained(this),
                     url),
      kMaxBodySize);
}

void WaybackMachineURLFetcher::OnWaybackURLFetched(
    const GURL& original_url,
    std::unique_ptr<std::string> response_body) {
  if (!response_body) {
    client_->OnWaybackURLFetched(GURL::EmptyGURL());
    return;
  }

  std::string wayback_response_json = std::move(*response_body);
  const auto result = base::JSONReader::Read(wayback_response_json);
  if (!result || !result->FindPath("archived_snapshots.closest.url")) {
    client_->OnWaybackURLFetched(GURL::EmptyGURL());
    return;
  }

  client_->OnWaybackURLFetched(
      GURL(result->FindPath("archived_snapshots.closest.url")->GetString()));
}
