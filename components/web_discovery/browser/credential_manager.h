/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_MANAGER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/web_discovery/browser/anonymous_credentials/rs/src/lib.rs.h"
#include "brave/components/web_discovery/browser/rsa.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "net/base/backoff_entry.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace web_discovery {

class CredentialManager {
 public:
  CredentialManager(PrefService* profile_prefs,
                    network::SharedURLLoaderFactory* shared_url_loader_factory,
                    std::unique_ptr<ServerConfig>* last_loaded_server_config,
                    base::RepeatingClosure credentials_loaded_callback);
  ~CredentialManager();

  CredentialManager(const CredentialManager&) = delete;
  CredentialManager& operator=(const CredentialManager&) = delete;

  void JoinGroups();

  std::optional<std::vector<uint8_t>> Sign(
      const std::vector<uint8_t>& msg,
      const std::vector<uint8_t>& basename);

 private:
  bool LoadRSAKey();
  bool GenerateRSAKey();

  void StartJoinGroup(const std::string& date,
                      const std::string& group_pub_key_b64);

  void OnJoinResponse(std::string date,
                      std::vector<uint8_t> group_pub_key,
                      std::vector<uint8_t> gsk,
                      std::optional<std::string> response_body);
  bool ProcessJoinResponse(const std::string& date,
                           const std::vector<uint8_t>& group_pub_key,
                           const std::vector<uint8_t>& gsk,
                           const std::optional<std::string>& response_body);

  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<std::unique_ptr<ServerConfig>> last_loaded_server_config_;

  GURL join_url_;
  base::flat_map<std::string, std::unique_ptr<network::SimpleURLLoader>>
      join_url_loaders_;
  net::BackoffEntry backoff_entry_;
  base::WallClockTimer retry_timer_;

  base::RepeatingClosure credentials_loaded_callback_;

  rust::Box<anonymous_credentials::CredentialManager>
      anonymous_credential_manager_;

  EVPKey rsa_private_key_;
  std::optional<std::string> rsa_public_key_b64_;

  std::optional<std::string> loaded_credential_date_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_MANAGER_H_
