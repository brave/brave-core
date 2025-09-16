/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_OBLIVIOUS_HTTP_KEY_CONFIG_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_OBLIVIOUS_HTTP_KEY_CONFIG_H_

#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "url/gurl.h"

namespace base {
class TimeDelta;
class Time;
}  // namespace base

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_ads {

// This class is responsible for managing the config key for sending Oblivious
// HTTP requests.
class NetworkClientObliviousHttpKeyConfig {
 public:
  NetworkClientObliviousHttpKeyConfig(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      GURL key_config_url);

  ~NetworkClientObliviousHttpKeyConfig();

  // Fetch the key config.
  void Fetch();

  // Returns the fetched key config, or `nullopt` if invalid or not yet fetched.
  std::optional<std::string> KeyConfig() const { return key_config_; }

 private:
  void FetchAfter(base::TimeDelta delay);
  void FetchCallback(network::SimpleURLLoader* loader,
                     std::optional<std::string> response_body);
  void SuccessfullyFetched(const std::string& key_config);
  void FailedToFetch();

  scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // Not owned.
  base::flat_set<std::unique_ptr<network::SimpleURLLoader>,
                 base::UniquePtrComparator>
      url_loaders_;

  const GURL key_config_url_;
  std::optional<std::string> key_config_;

  bool is_fetching_ = false;
  base::WallClockTimer fetch_timer_;
  base::TimeDelta backoff_delay_;

  base::WeakPtrFactory<NetworkClientObliviousHttpKeyConfig> weak_ptr_factory_{
      this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_OBLIVIOUS_HTTP_KEY_CONFIG_H_
