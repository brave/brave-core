/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_KEY_CONFIG_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_KEY_CONFIG_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/brave_ads/core/browser/service/oblivious_http_feature.h"
#include "url/gurl.h"

class PrefService;

namespace base {
class TimeDelta;
}  // namespace base

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_ads {

// Manages the key config required for performing Oblivious HTTP (OHTTP)
// requests. This includes fetching, caching, validating expiration, and
// retry/backoff handling.
class ObliviousHttpKeyConfig {
 public:
  ObliviousHttpKeyConfig(
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      GURL key_config_url);

  ~ObliviousHttpKeyConfig();

  // Fetches the key config if it is expired or has not yet been fetched.
  void MaybeFetch();

  // Returns the cached key config, or `std::nullopt` if it is invalid or has
  // not yet been retrieved.
  std::optional<std::string> Get() const;

  // Invalidates the cached key config and triggers an immediate fetch.
  void InvalidateAndFetch();

 private:
  base::TimeDelta ExpiresAfter() const;
  bool HasExpired() const;

  void FetchAfter(base::TimeDelta delay);
  void Fetch();
  void FetchCallback(std::unique_ptr<network::SimpleURLLoader> url_loader,
                     std::optional<std::string> response_body);
  void SuccessfullyFetched(const std::string& key_config);
  void FailedToFetch();
  void Retry();

  void Update(const std::string& key_config);
  void Invalidate();

  const raw_ptr<PrefService> local_state_;  // Not owned.

  scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // Not owned.

  const GURL key_config_url_;

  bool is_fetching_ = false;
  base::WallClockTimer fetch_timer_;
  base::TimeDelta backoff_delay_ = kOhttpKeyConfigInitialBackoffDelay.Get();

  base::WeakPtrFactory<ObliviousHttpKeyConfig> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_KEY_CONFIG_H_
