/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client_oblivious_http_key_config.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_feature.h"
#include "brave/components/brave_ads/core/browser/service/network_client_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kInitialBackoffDelay = base::Minutes(1);
constexpr base::TimeDelta kMaxBackoffDelay = base::Hours(1);

}  // namespace

NetworkClientObliviousHttpKeyConfig::NetworkClientObliviousHttpKeyConfig(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    GURL key_config_url)
    : url_loader_factory_(std::move(url_loader_factory)),
      key_config_url_(std::move(key_config_url)),
      backoff_delay_(kInitialBackoffDelay) {}

NetworkClientObliviousHttpKeyConfig::~NetworkClientObliviousHttpKeyConfig() =
    default;

void NetworkClientObliviousHttpKeyConfig::Fetch() {
  if (is_fetching_) {
    // A fetch is already in progress.
    return;
  }

  is_fetching_ = true;

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = key_config_url_;
  resource_request->method = net::HttpRequestHeaders::kGetMethod;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());

  url_loader->SetAllowHttpErrorResults(false);

  auto* url_loader_copy = url_loader.get();
  url_loaders_.insert(std::move(url_loader));

  url_loader_copy->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&NetworkClientObliviousHttpKeyConfig::FetchCallback,
                     weak_ptr_factory_.GetWeakPtr(), url_loader_copy));
}

///////////////////////////////////////////////////////////////////////////////

void NetworkClientObliviousHttpKeyConfig::FetchAfter(base::TimeDelta delay) {
  const int jitter = base::RandInt(0, 60);
  fetch_timer_.Start(FROM_HERE,
                     base::Time::Now() + delay + base::Seconds(jitter),
                     base::BindOnce(&NetworkClientObliviousHttpKeyConfig::Fetch,
                                    weak_ptr_factory_.GetWeakPtr()));
}

void NetworkClientObliviousHttpKeyConfig::FetchCallback(
    network::SimpleURLLoader* url_loader,
    std::optional<std::string> url_response_body) {
  CHECK(url_loader);

  auto iter = url_loaders_.find(url_loader);
  CHECK(iter != url_loaders_.cend());
  auto owned_url_loader = std::move(*iter);
  url_loaders_.erase(iter);

  if (!url_response_body) {
    return FailedToFetch();
  }

  const auto* response = owned_url_loader->ResponseInfo();
  if (!response || !response->headers) {
    return FailedToFetch();
  }

  const auto response_headers = response->headers;
  if (response_headers->response_code() != net::HTTP_OK) {
    return FailedToFetch();
  }

  // TODO(tmancey): If the key is invalid should we force an update?
  const auto key_config =
      std::string(url_response_body->cbegin(), url_response_body->cend());

  SuccessfullyFetched(key_config);
}

void NetworkClientObliviousHttpKeyConfig::SuccessfullyFetched(
    const std::string& key_config) {
  // TODO(tmancey): Consider persisting the key config and its expiry date to
  // only fetch if expired.
  key_config_ = key_config;

  backoff_delay_ = kInitialBackoffDelay;

  is_fetching_ = false;

  FetchAfter(kFetchOhttpKeyConfigAfter.Get());
}

void NetworkClientObliviousHttpKeyConfig::FailedToFetch() {
  FetchAfter(backoff_delay_);

  // Exponential backoff, capped at `kMaxBackoffDelay`.
  backoff_delay_ = std::min(backoff_delay_ * 2, kMaxBackoffDelay);
}

}  // namespace brave_ads
