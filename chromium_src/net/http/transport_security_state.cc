/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

#define BRAVE_ENABLE_STATIC_PINS \
  enable_static_pins_ = true;    \
  enable_static_expect_ct_ = true;

#else

// Leave static pins disabled on Android and iOS, like upstream.
#define BRAVE_ENABLE_STATIC_PINS \
  {}

#endif

#define BRAVE_TRANSPORT_SECURITY_STATE_DELETE_DYNAMIC_DATA_FOR_HOST \
  if (enabled_sts_hosts_.DeleteDataInAllPartitions(hashed_host)) {  \
    deleted = true;                                                 \
  }

#include "src/net/http/transport_security_state.cc"

#undef BRAVE_TRANSPORT_SECURITY_STATE_DELETE_DYNAMIC_DATA_FOR_HOST
#undef BRAVE_ENABLE_STATIC_PINS

namespace net {

namespace {

// Use only top frame site as a key for HSTS partitioning to not over-populate
// HSTS state storage.
absl::optional<std::string> GetTopFrameSiteHash(
    const NetworkIsolationKey& network_isolation_key) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return absl::nullopt;
  }

  // An empty or opaque top frame site cannot be used as a partition key, return
  // an empty string which will be treated as a non-persistable partition.
  if (network_isolation_key.IsTransient() ||
      !network_isolation_key.GetTopFrameSite().has_value() ||
      network_isolation_key.GetTopFrameSite()->opaque()) {
    return std::string();
  }

  return HashHost(network_isolation_key.GetTopFrameSite()->Serialize());
}

}  // namespace

bool TransportSecurityState::ShouldSSLErrorsBeFatal(
    const NetworkIsolationKey& network_isolation_key,
    const std::string& host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetPartitionHash(
      GetTopFrameSiteHash(network_isolation_key));
  return ShouldSSLErrorsBeFatal(host);
}

bool TransportSecurityState::ShouldUpgradeToSSL(
    const NetworkIsolationKey& network_isolation_key,
    const std::string& host,
    const NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetPartitionHash(
      GetTopFrameSiteHash(network_isolation_key));
  return ShouldUpgradeToSSL(host, net_log);
}

bool TransportSecurityState::AddHSTSHeader(
    const NetworkIsolationKey& network_isolation_key,
    const std::string& host,
    const std::string& value) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetPartitionHash(
      GetTopFrameSiteHash(network_isolation_key));
  return AddHSTSHeader(host, value);
}

}  // namespace net
