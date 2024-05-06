/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "net/base/backoff_entry.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace web_discovery {

using KeyMap = base::flat_map<std::string, std::string>;

struct ServerConfig {
  ServerConfig();
  ~ServerConfig();

  KeyMap group_pub_keys;
  KeyMap pub_keys;
};

class ServerConfigLoader {
 public:
  using ConfigCallback =
      base::RepeatingCallback<void(std::unique_ptr<ServerConfig>)>;

  explicit ServerConfigLoader(
      network::SharedURLLoaderFactory* shared_url_loader_factory,
      ConfigCallback config_callback);
  ~ServerConfigLoader();

  ServerConfigLoader(const ServerConfigLoader&) = delete;
  ServerConfigLoader& operator=(const ServerConfigLoader&) = delete;

  void Load();

 private:
  void OnConfigResponse(std::optional<std::string> response_body);
  bool ProcessConfigResponse(const std::optional<std::string>& response_body);

  GURL config_url_;
  raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  ConfigCallback config_callback_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  net::BackoffEntry backoff_entry_;

  base::WallClockTimer update_timer_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_
