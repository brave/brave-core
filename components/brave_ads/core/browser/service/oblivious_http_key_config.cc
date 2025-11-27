/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/oblivious_http_key_config.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/browser/service/network_client_util.h"
#include "brave/components/brave_ads/core/browser/service/oblivious_http_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave_ads {

ObliviousHttpKeyConfig::ObliviousHttpKeyConfig(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    GURL key_config_url)
    : local_state_(local_state),
      url_loader_factory_(std::move(url_loader_factory)),
      key_config_url_(std::move(key_config_url)) {
  CHECK(local_state_);
}

ObliviousHttpKeyConfig::~ObliviousHttpKeyConfig() = default;

void ObliviousHttpKeyConfig::MaybeFetch() {
  if (HasExpired()) {
    VLOG(6) << "OHTTP key config has expired";
    return Fetch();
  }

  FetchAfter(kOhttpKeyConfigExpiresAfter.Get());
}

void ObliviousHttpKeyConfig::InvalidateAndFetch() {
  Invalidate();
  Fetch();
}

std::optional<std::string> ObliviousHttpKeyConfig::Get() const {
  if (!local_state_->HasPrefPath(prefs::kObliviousHttpKeyConfig)) {
    // Fresh install, so force a fetch.
    return std::nullopt;
  }

  const auto key_config = base::Base64Decode(
      local_state_->GetString(prefs::kObliviousHttpKeyConfig));
  if (!key_config) {
    // Invalid key config, so force a fetch.
    return std::nullopt;
  }

  return std::string(key_config->cbegin(), key_config->cend());
}

///////////////////////////////////////////////////////////////////////////////

base::TimeDelta ObliviousHttpKeyConfig::ExpiresAfter() const {
  if (HasExpired()) {
    // Already expired.
    return base::TimeDelta::Min();
  }

  return local_state_->GetTime(prefs::kObliviousHttpKeyConfigExpiresAt) -
         base::Time::Now();
}

bool ObliviousHttpKeyConfig::HasExpired() const {
  return base::Time::Now() >=
         local_state_->GetTime(prefs::kObliviousHttpKeyConfigExpiresAt);
}

void ObliviousHttpKeyConfig::FetchAfter(base::TimeDelta delay) {
  // Randomized delay to prevent timing correlation.
  const base::TimeDelta randomized_delay =
      delay + base::Seconds(base::RandInt(0, 60));
  const base::Time fetch_at = base::Time::Now() + randomized_delay;
  VLOG(6) << "Fetch OHTTP key config at " << fetch_at;
  fetch_timer_.Start(FROM_HERE, fetch_at,
                     base::BindOnce(&ObliviousHttpKeyConfig::Fetch,
                                    weak_ptr_factory_.GetWeakPtr()));
}

void ObliviousHttpKeyConfig::Fetch() {
  if (is_fetching_) {
    // Fetch already in progress.
    return;
  }
  is_fetching_ = true;

  VLOG(6) << "Fetching OHTTP key config";

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = key_config_url_;
  resource_request->method = net::HttpRequestHeaders::kGetMethod;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());
  auto* raw_url_loader = url_loader.get();

  raw_url_loader->SetAllowHttpErrorResults(true);

  raw_url_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&ObliviousHttpKeyConfig::FetchCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(url_loader)));
}

void ObliviousHttpKeyConfig::FetchCallback(
    std::unique_ptr<network::SimpleURLLoader> url_loader,
    std::optional<std::string> url_response_body) {
  CHECK(url_loader);

  if (!url_response_body) {
    VLOG(6) << "Failed to fetch OHTTP key config due to missing response body";
    return FailedToFetch();
  }

  const auto* response = url_loader->ResponseInfo();
  if (!response) {
    VLOG(6) << "Failed to fetch OHTTP key config due to missing response";
    return FailedToFetch();
  }

  if (!response->headers) {
    VLOG(6)
        << "Failed to fetch OHTTP key config due to missing response headers";
    return FailedToFetch();
  }

  const auto response_headers = response->headers;
  if (response_headers->response_code() != net::HTTP_OK) {
    VLOG(6) << "Failed to fetch OHTTP key config due to invalid response code: "
            << response_headers->response_code();
    return FailedToFetch();
  }

  SuccessfullyFetched(*url_response_body);
}

void ObliviousHttpKeyConfig::SuccessfullyFetched(
    const std::string& key_config) {
  VLOG(6) << "Successfully fetched OHTTP key config";

  backoff_delay_ = kOhttpKeyConfigInitialBackoffDelay.Get();

  is_fetching_ = false;

  Update(key_config);

  FetchAfter(ExpiresAfter());
}

void ObliviousHttpKeyConfig::FailedToFetch() {
  Retry();
}

void ObliviousHttpKeyConfig::Retry() {
  VLOG(6) << "Retry fetching OHTTP key config";

  FetchAfter(backoff_delay_);

  // Exponential backoff, capped at `kOhttpKeyConfigMaxBackoffDelay`.
  backoff_delay_ =
      std::min(backoff_delay_ * 2, kOhttpKeyConfigMaxBackoffDelay.Get());
}

void ObliviousHttpKeyConfig::Update(const std::string& key_config) {
  local_state_->SetString(prefs::kObliviousHttpKeyConfig,
                          base::Base64Encode(key_config));
  local_state_->SetTime(prefs::kObliviousHttpKeyConfigExpiresAt,
                        base::Time::Now() + kOhttpKeyConfigExpiresAfter.Get());
}

void ObliviousHttpKeyConfig::Invalidate() {
  VLOG(6) << "Invalidating OHTTP key config";
  local_state_->ClearPref(prefs::kObliviousHttpKeyConfig);
  local_state_->ClearPref(prefs::kObliviousHttpKeyConfigExpiresAt);
}

}  // namespace brave_ads
