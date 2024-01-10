/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_REQUEST_SIGNER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_REQUEST_SIGNER_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/common/signer.h"

namespace brave_rewards::internal {

// Responsible for signing server API requests with the user's secret key.
class RequestSigner {
 public:
  // Returns the digest header value for the specified content.
  static std::string GetDigest(base::span<const uint8_t> content);

  // Returns a `RequestSigner` for the specified `RewardsWallet`. The
  // `recovery_seed` is used to derive the signing key and the `payment_id` is
  // used as the request key. The request key can be modified using
  // `set_key_id`.
  static std::optional<RequestSigner> FromRewardsWallet(
      const mojom::RewardsWallet& rewards_wallet);

  ~RequestSigner();

  RequestSigner(const RequestSigner&);
  RequestSigner& operator=(const RequestSigner&);

  const std::string& key_id() const { return key_id_; }
  void set_key_id(const std::string& key_id) { key_id_ = key_id; }

  // Returns the `Signer` used to sign requests.
  const Signer& signer() const { return signer_; }

  // Adds signature headers to the specified request. Returns `false` if the
  // request cannot be signed (e.g. if the request has an invalid URL).
  [[nodiscard]] bool SignRequest(mojom::UrlRequest& request);

  // Generates the signature headers for the specified request details.
  std::vector<std::string> GetSignedHeaders(const std::string& request_path,
                                            const std::string& request_content);

  // Generates a signature for the specified collection of headers.
  std::string SignHeaders(
      base::span<const std::pair<std::string, std::string>> headers);

 private:
  RequestSigner(const std::string& key_id, Signer signer);

  std::string key_id_;
  Signer signer_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_REQUEST_SIGNER_H_
