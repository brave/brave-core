/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_SIGNER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_SIGNER_H_

#include <optional>
#include <vector>

#include "base/containers/span.h"

namespace brave_rewards::internal {

// Responsible for signing messages with a key pair derived from the user's
// "recovery seed", typically stored with the user's browser profile.
class Signer {
 public:
  // Returns a `Signer` derived from the specified recovery seed. If the
  // recovery seed is invalid (e.g. the length of the seed is incorrect), a
  // `nullopt` is returned.
  static std::optional<Signer> FromRecoverySeed(
      const std::vector<uint8_t>& recovery_seed);

  // Generates a new recovery seed.
  static std::vector<uint8_t> GenerateRecoverySeed();

  ~Signer();

  Signer(const Signer&);
  Signer& operator=(const Signer&);

  const std::vector<uint8_t> public_key() const { return public_key_; }
  const std::vector<uint8_t> secret_key() const { return secret_key_; }

  // Signs the specified message using the signer's secret key.
  std::vector<uint8_t> SignMessage(base::span<const uint8_t> message);

 private:
  Signer(std::vector<uint8_t> public_key, std::vector<uint8_t> secret_key);

  std::vector<uint8_t> public_key_;
  std::vector<uint8_t> secret_key_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_SIGNER_H_
