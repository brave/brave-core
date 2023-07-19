/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_META_H_
#define BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_META_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/p3a/constellation/rs/cxx/src/lib.rs.h"
#include "brave/components/p3a/p3a_config.h"
#include "net/base/hash_value.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;
class PrefRegistrySimple;

namespace net {
class X509Certificate;
}  // namespace net

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace p3a {

struct RandomnessServerInfo {
  RandomnessServerInfo(
      uint8_t current_epoch,
      base::Time next_epoch_time,
      bool epoch_change_detected,
      ::rust::Box<constellation::PPOPRFPublicKeyWrapper> public_key);
  ~RandomnessServerInfo();

  uint8_t current_epoch;
  base::Time next_epoch_time;
  bool epoch_change_detected;
  ::rust::Box<constellation::PPOPRFPublicKeyWrapper> public_key;
};

// Handles retrieval of the current epoch number and next epoch time
// from the randomness host. Also handles Nitro Enclave attestation
// verification.
class StarRandomnessMeta {
 public:
  using RandomnessServerInfoCallback =
      base::RepeatingCallback<void(RandomnessServerInfo* server_info)>;

  StarRandomnessMeta(
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      RandomnessServerInfoCallback info_callback,
      const P3AConfig* config);
  ~StarRandomnessMeta();

  StarRandomnessMeta(const StarRandomnessMeta&) = delete;
  StarRandomnessMeta& operator=(const StarRandomnessMeta&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  bool VerifyRandomnessCert(network::SimpleURLLoader* url_loader);

  void RequestServerInfo();

  RandomnessServerInfo* GetCachedRandomnessServerInfo();

 private:
  bool ShouldAttestEnclave();

  void AttestServer(bool make_info_request_after);

  void HandleServerInfoResponse(std::unique_ptr<std::string> response_body);

  void HandleAttestationResult(
      bool make_info_request_after,
      scoped_refptr<net::X509Certificate> approved_cert);

  void ScheduleServerInfoRetry();

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  std::unique_ptr<RandomnessServerInfo> rnd_server_info_;

  bool has_used_cached_info_ = false;
  absl::optional<uint8_t> last_cached_epoch_ = absl::nullopt;

  const raw_ptr<PrefService> local_state_;

  base::OneShotTimer rnd_info_retry_timer_;
  base::TimeDelta current_backoff_time_;
  RandomnessServerInfoCallback info_callback_;

  const raw_ptr<const P3AConfig> config_;

  absl::optional<net::HashValue> approved_cert_fp_;
  bool attestation_pending_ = false;

  base::WeakPtrFactory<StarRandomnessMeta> weak_ptr_factory_{this};
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_META_H_
