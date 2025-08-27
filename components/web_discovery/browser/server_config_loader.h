/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/patterns_v2.h"
#include "net/base/backoff_entry.h"
#include "url/gurl.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace web_discovery {

using KeyMap = base::flat_map<std::string, std::vector<uint8_t>>;
using ParsedPatternsVariant = std::variant<std::unique_ptr<PatternsGroup>,
                                           std::unique_ptr<V2PatternsGroup>>;

struct SourceMapActionConfig {
  SourceMapActionConfig();
  ~SourceMapActionConfig();

  SourceMapActionConfig(const SourceMapActionConfig&) = delete;
  SourceMapActionConfig& operator=(const SourceMapActionConfig&) = delete;

  std::vector<std::string> keys;
  size_t limit;
  size_t period;
};

struct ServerConfig {
  ServerConfig();
  ~ServerConfig();

  ServerConfig(const ServerConfig&) = delete;
  ServerConfig& operator=(const ServerConfig&) = delete;

  KeyMap group_pub_keys;
  KeyMap pub_keys;

  base::flat_map<std::string, std::unique_ptr<SourceMapActionConfig>>
      source_map_actions;

  std::string location;
};

struct ServerConfigDownloadResult {
  ServerConfigDownloadResult(bool is_collector_config,
                             std::optional<std::string> response_body);
  ~ServerConfigDownloadResult();

  ServerConfigDownloadResult(const ServerConfigDownloadResult&);

  bool is_collector_config;
  std::optional<std::string> response_body;
};

// Handles retrieval, updating and caching of the following server
// configurations:
// - HPN server config: contains public keys, and "source maps" used
//   for generating basenames.
// - "quorum" config: contains the country code of the user
// - patterns: contains the rules for scraping/submission of certain pages
class ServerConfigLoader {
 public:
  ServerConfigLoader(PrefService* local_state,
                     base::FilePath user_data_dir,
                     network::SharedURLLoaderFactory* shared_url_loader_factory,
                     base::RepeatingClosure config_callback,
                     base::RepeatingClosure patterns_callback);
  ~ServerConfigLoader();

  ServerConfigLoader(const ServerConfigLoader&) = delete;
  ServerConfigLoader& operator=(const ServerConfigLoader&) = delete;

  // Loads all three server configurations. Update requests will be scheduled
  // once complete.
  void LoadConfigs();

  // Returns the last loaded server config, which is a combination of the
  // HPN and "quorum" configs. May only call after the config_callback is
  // triggered.
  const ServerConfig& GetLastServerConfig() const;
  // Returns the pattern config. May only call after the patterns_callback is
  // triggered.
  const PatternsGroup& GetLastPatterns() const;
  // Returns the v2 pattern config. May only call after the patterns_callback is
  // triggered.
  const V2PatternsGroup& GetLastV2Patterns() const;

  void SetLastServerConfigForTesting(
      std::unique_ptr<ServerConfig> server_config);
  void SetLastPatternsForTesting(std::unique_ptr<PatternsGroup> patterns);
  void SetLastV2PatternsForTesting(
      std::unique_ptr<V2PatternsGroup> v2_patterns);

 private:
  void OnConfigResponsesDownloaded(
      std::vector<ServerConfigDownloadResult> results);
  void OnConfigResponsesProcessed(std::unique_ptr<ServerConfig> config);

  void LoadStoredPatterns();
  void SchedulePatternsRequest();
  void RequestPatterns();
  void OnPatternsResponse(std::optional<std::string> response_body);
  void OnPatternsParsed(bool is_stored, ParsedPatternsVariant patterns);
  void HandlePatternsStatus(bool result);

  const raw_ptr<PrefService> local_state_;

  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  GURL collector_config_url_;
  GURL quorum_config_url_;
  GURL patterns_url_;
  base::FilePath patterns_path_;
  raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  base::RepeatingClosure config_callback_;
  base::RepeatingClosure patterns_callback_;

  std::unique_ptr<network::SimpleURLLoader> collector_config_url_loader_;
  std::unique_ptr<network::SimpleURLLoader> quorum_config_url_loader_;
  std::unique_ptr<network::SimpleURLLoader> patterns_url_loader_;
  net::BackoffEntry config_backoff_entry_;
  net::BackoffEntry patterns_backoff_entry_;

  base::WallClockTimer config_update_timer_;
  base::WallClockTimer patterns_update_timer_;
  bool patterns_first_request_made_ = false;

  std::unique_ptr<ServerConfig> last_loaded_server_config_;
  std::unique_ptr<PatternsGroup> last_loaded_patterns_;
  std::unique_ptr<V2PatternsGroup> last_loaded_v2_patterns_;

  base::WeakPtrFactory<ServerConfigLoader> weak_ptr_factory_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SERVER_CONFIG_LOADER_H_
