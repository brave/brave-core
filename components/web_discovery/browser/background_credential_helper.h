/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "crypto/keypair.h"

namespace web_discovery {

struct StartJoinInitialization {
  StartJoinInitialization(std::string request_b64,
                          std::vector<uint8_t> gsk,
                          std::string signature);

  ~StartJoinInitialization();

  StartJoinInitialization(const StartJoinInitialization&);
  StartJoinInitialization& operator=(const StartJoinInitialization&);

  // The encoded join request to be sent to the Web Discovery server.
  std::string request_b64;
  // The generated secret key for the credential.
  std::vector<uint8_t> gsk;
  // The signature of the join request.
  std::string signature;
};

class BackgroundCredentialHelper {
 public:
  static std::unique_ptr<BackgroundCredentialHelper> Create();

  virtual ~BackgroundCredentialHelper() = default;

  // Use a fixed seed for cryptographic operations
  virtual void UseFixedSeedForTesting() = 0;

  // Generate a new RSA key, store the key internally for future operations
  // and return the new key
  virtual crypto::keypair::PrivateKey GenerateAndSetRSAKey() = 0;

  // Store an imported key for future operations
  virtual void SetRSAKey(crypto::keypair::PrivateKey rsa_private_key) = 0;

  // Generate a join request to be sent to the Web Discovery server.
  virtual StartJoinInitialization GenerateJoinRequest(
      std::string pre_challenge) = 0;

  // Process a response from the server for a join request to finish
  // the join process. Messages can be signed once complete.
  virtual std::optional<std::string> FinishJoin(
      std::string date,
      std::vector<uint8_t> group_pub_key,
      std::vector<uint8_t> gsk,
      std::vector<uint8_t> join_resp_bytes) = 0;

  // Sign a message given the group secret key and credential, for transmission
  // to the Web Discovery server.
  virtual std::optional<std::vector<uint8_t>> PerformSign(
      std::vector<uint8_t> msg,
      std::vector<uint8_t> basename,
      std::optional<std::vector<uint8_t>> gsk_bytes,
      std::optional<std::vector<uint8_t>> credential_bytes) = 0;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_H_
