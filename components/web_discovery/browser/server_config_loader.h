/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "net/base/backoff_entry.h"
#include "url/gurl.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace web_discovery {

using KeyMap = base::flat_map<std::string, std::string>;

struct SourceMapActionConfig {
  SourceMapActionConfig();
  ~SourceMapActionConfig();

  SourceMapActionConfig(const SourceMapActionConfig&);

  std::vector<std::string> keys;
  size_t limit;
  size_t period;
};

struct ServerConfig {
  ServerConfig();
  ~ServerConfig();

  KeyMap group_pub_keys;
  KeyMap pub_keys;

  base::flat_map<std::string, SourceMapActionConfig> source_map_actions;

  std::string location;
};

class ServerConfigLoader {
 public:
  using ConfigCallback =
      base::RepeatingCallback<void(std::unique_ptr<ServerConfig>)>;
  using PatternsCallback =
      base::RepeatingCallback<void(std::unique_ptr<PatternsGroup>)>;

  explicit ServerConfigLoader(
      PrefService* local_state,
      base::FilePath user_data_dir,
      network::SharedURLLoaderFactory* shared_url_loader_factory,
      ConfigCallback config_callback,
      PatternsCallback patterns_callback);
  ~ServerConfigLoader();

  ServerConfigLoader(const ServerConfigLoader&) = delete;
  ServerConfigLoader& operator=(const ServerConfigLoader&) = delete;

 private:
  void LoadConfigs();
  void OnConfigResponses(
      std::vector<std::optional<std::string>> response_bodies);
  bool ProcessConfigResponses(
      const std::optional<std::string>& collector_response_body,
      const std::optional<std::string>& quorum_response_body);

  void LoadStoredPatterns();
  void OnPatternsFileLoaded(std::optional<std::string> patterns_json);
  void SchedulePatternsRequest();
  void RequestPatterns();
  void OnPatternsResponse(std::optional<std::string> response_body);
  void OnPatternsGunzip(std::optional<std::string> patterns_json);
  void OnPatternsWritten(std::unique_ptr<PatternsGroup> parsed_group,
                         bool result);
  void HandlePatternsStatus(bool result);

  raw_ptr<PrefService> local_state_;

  scoped_refptr<base::SequencedTaskRunner> pool_sequenced_task_runner_;

  GURL collector_config_url_;
  GURL quorum_config_url_;
  GURL patterns_url_;
  base::FilePath patterns_path_;
  raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  ConfigCallback config_callback_;
  PatternsCallback patterns_callback_;

  std::unique_ptr<network::SimpleURLLoader> collector_config_url_loader_;
  std::unique_ptr<network::SimpleURLLoader> quorum_config_url_loader_;
  std::unique_ptr<network::SimpleURLLoader> patterns_url_loader_;
  net::BackoffEntry config_backoff_entry_;
  net::BackoffEntry patterns_backoff_entry_;

  base::WallClockTimer config_update_timer_;
  base::WallClockTimer patterns_update_timer_;
  bool patterns_first_request_made_ = false;

  base::WeakPtrFactory<ServerConfigLoader> weak_ptr_factory_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_
