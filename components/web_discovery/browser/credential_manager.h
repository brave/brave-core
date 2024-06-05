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
#include "base/task/sequenced_task_runner.h"
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

struct GenerateJoinRequestResult {
  anonymous_credentials::StartJoinResult start_join_result;
  std::string signature;
};

class CredentialManager {
 public:
  using SignCallback =
      base::OnceCallback<void(std::optional<std::vector<const uint8_t>>)>;
  CredentialManager(PrefService* profile_prefs,
                    network::SharedURLLoaderFactory* shared_url_loader_factory,
                    std::unique_ptr<ServerConfig>* last_loaded_server_config);
  ~CredentialManager();

  CredentialManager(const CredentialManager&) = delete;
  CredentialManager& operator=(const CredentialManager&) = delete;

  void JoinGroups();

  bool CredentialExistsForToday();
  bool Sign(std::vector<const uint8_t> msg,
            std::vector<const uint8_t> basename,
            SignCallback callback);

 private:
  bool LoadRSAKey();
  bool GenerateRSAKey();
  void OnNewRSAKey(std::unique_ptr<RSAKeyInfo> key_info);

  void StartJoinGroup(const std::string& date,
                      const std::string& group_pub_key_b64);

  void OnJoinRequestReady(
      std::string date,
      std::vector<const uint8_t> group_pub_key,
      std::optional<GenerateJoinRequestResult> generate_join_result);

  void OnJoinResponse(std::string date,
                      std::vector<const uint8_t> group_pub_key,
                      std::vector<const uint8_t> gsk,
                      std::optional<std::string> response_body);
  void HandleJoinResponseStatus(const std::string& date, bool result);
  bool ProcessJoinResponse(const std::string& date,
                           const std::vector<const uint8_t>& group_pub_key,
                           const std::vector<const uint8_t>& gsk,
                           const std::optional<std::string>& response_body);
  void OnCredentialsReady(std::string date,
                          std::vector<const uint8_t> gsk,
                          std::optional<std::string> credentials);

  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<std::unique_ptr<ServerConfig>> last_loaded_server_config_;

  GURL join_url_;
  base::flat_map<std::string, std::unique_ptr<network::SimpleURLLoader>>
      join_url_loaders_;
  net::BackoffEntry backoff_entry_;
  base::WallClockTimer retry_timer_;

  scoped_refptr<base::SequencedTaskRunner> pool_sequenced_task_runner_;

  std::unique_ptr<rust::Box<anonymous_credentials::CredentialManager>,
                  base::OnTaskRunnerDeleter>
      anonymous_credential_manager_;

  std::unique_ptr<EVPKeyPtr, base::OnTaskRunnerDeleter> rsa_private_key_;
  std::optional<std::string> rsa_public_key_b64_;

  std::optional<std::string> loaded_credential_date_;

  base::WeakPtrFactory<CredentialManager> weak_ptr_factory_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_MANAGER_H_
