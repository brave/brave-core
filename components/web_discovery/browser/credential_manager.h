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

#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/web_discovery/browser/background_credential_helper.h"
#include "brave/components/web_discovery/browser/credential_signer.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "net/base/backoff_entry.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace web_discovery {

// Manages and utilizes anonymous credentials used for communicating
// with Web Discovery servers. These Direct Anonymous Attestation credentials
// are used to prevent Sybil attacks on the servers.
// The manager provides two key functions:
//
// a) "joining": acquires credentials from the Web Discovery server. Join
// requests
//    are signed with a random RSA key that is persisted with the profile.
// b) "signing": uses the previously acquired credentials to sign submissions
//    which is required in order for the servers to accept the request.
class CredentialManager : public CredentialSigner {
 public:
  CredentialManager(PrefService* profile_prefs,
                    network::SharedURLLoaderFactory* shared_url_loader_factory,
                    const ServerConfigLoader* server_config_loader);
  ~CredentialManager() override;

  CredentialManager(const CredentialManager&) = delete;
  CredentialManager& operator=(const CredentialManager&) = delete;

  // Acquires credentials for all dates/"group public keys" published in
  // the server config, if not stored already.
  void JoinGroups();

  // CredentialSigner:
  bool CredentialExistsForToday() override;

  void Sign(std::vector<const uint8_t> msg,
            std::vector<const uint8_t> basename,
            SignCallback callback) override;

  // Uses a fixed seed in the anonymous credential manager
  // to provide deterministic credentials & signatures which
  // are useful for testing.
  void UseFixedSeedForTesting();

 private:
  bool LoadRSAKey();
  bool GenerateRSAKey();
  void OnNewRSAKey(std::unique_ptr<RSAKeyInfo> key_info);

  void StartJoinGroup(const std::string& date,
                      const std::vector<uint8_t>& group_pub_key);

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

  void OnSignResult(std::string credential_date,
                    SignCallback callback,
                    std::optional<std::vector<const uint8_t>> signed_message);

  const raw_ptr<PrefService> profile_prefs_;
  const raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  const raw_ptr<const ServerConfigLoader> server_config_loader_;

  GURL join_url_;
  base::flat_map<std::string, std::unique_ptr<network::SimpleURLLoader>>
      join_url_loaders_;
  net::BackoffEntry backoff_entry_;
  base::WallClockTimer retry_timer_;

  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  base::SequenceBound<BackgroundCredentialHelper> background_credential_helper_;
  std::optional<std::string> rsa_public_key_b64_;

  std::optional<std::string> loaded_credential_date_;

  base::WeakPtrFactory<CredentialManager> weak_ptr_factory_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_MANAGER_H_
