/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_RANDOMNESS_META_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_RANDOMNESS_META_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/nested_star/src/lib.rs.h"
#include "brave/components/p3a/brave_p3a_config.h"

class PrefService;
class PrefRegistrySimple;

namespace net {
class X509Certificate;
}  // namespace net

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave {

struct RandomnessServerInfo {
  RandomnessServerInfo(
      uint8_t current_epoch,
      base::Time next_epoch_time,
      ::rust::Box<nested_star::PPOPRFPublicKeyWrapper> public_key);
  ~RandomnessServerInfo();

  uint8_t current_epoch;
  base::Time next_epoch_time;
  ::rust::Box<nested_star::PPOPRFPublicKeyWrapper> public_key;
};

class BraveP3AStarRandomnessMeta {
 public:
  using RandomnessServerInfoCallback =
      base::RepeatingCallback<void(RandomnessServerInfo* server_info)>;

  BraveP3AStarRandomnessMeta(
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      RandomnessServerInfoCallback info_callback,
      BraveP3AConfig* config);
  ~BraveP3AStarRandomnessMeta();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  bool VerifyRandomnessCert(network::SimpleURLLoader* url_loader);

  void RequestServerInfo();

  RandomnessServerInfo* GetCachedRandomnessServerInfo();

 private:
  void AttestServer(bool make_info_request_after);

  void HandleServerInfoResponse(std::unique_ptr<std::string> response_body);

  void HandleAttestationResult(
      bool make_info_request_after,
      scoped_refptr<net::X509Certificate> approved_cert);

  void ScheduleServerInfoRetry();

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  std::unique_ptr<RandomnessServerInfo> rnd_server_info_;

  bool has_used_cached_info = false;

  PrefService* local_state_;

  base::OneShotTimer rnd_info_retry_timer_;
  base::TimeDelta current_backoff_time_;
  RandomnessServerInfoCallback info_callback_;

  BraveP3AConfig* config_;

  scoped_refptr<net::X509Certificate> approved_cert_;
  bool attestation_pending_ = false;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_RANDOMNESS_META_H_
