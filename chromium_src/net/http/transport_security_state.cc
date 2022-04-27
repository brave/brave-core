/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#include "build/build_config.h"
#include "url/gurl.h"

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

// Non-anonymous helper to friend with SchemefulSite.
class HSTSPartitionHashHelper {
 public:
  static absl::optional<std::string> GetSchemefulSiteRegistrableDomainOrHost(
      const SchemefulSite& schemeful_site) {
    if (!schemeful_site.has_registrable_domain_or_host()) {
      return absl::nullopt;
    }
    return schemeful_site.registrable_domain_or_host();
  }
};

namespace {

bool IsTopFrameOriginCryptographic(const IsolationInfo& isolation_info) {
  return isolation_info.top_frame_origin() &&
         GURL::SchemeIsCryptographic(
             isolation_info.top_frame_origin()->scheme());
}

absl::optional<std::string> GetHSTSPartitionHash(
    const NetworkIsolationKey& network_isolation_key) {
  DCHECK(base::FeatureList::IsEnabled(features::kBravePartitionHSTS));
  // An empty or opaque top frame site cannot be used as a partition key, return
  // a hash which will be treated as a non-persistable partition.
  if (network_isolation_key.IsTransient() ||
      !network_isolation_key.GetTopFrameSite().has_value() ||
      network_isolation_key.GetTopFrameSite()->opaque()) {
    return std::string();
  }

  absl::optional<std::string> top_frame_registrable_domain_or_host =
      HSTSPartitionHashHelper::GetSchemefulSiteRegistrableDomainOrHost(
          *network_isolation_key.GetTopFrameSite());
  if (!top_frame_registrable_domain_or_host) {
    return std::string();
  }

  return HashHost(*top_frame_registrable_domain_or_host);
}

// Use only top frame site as a key for HSTS partitioning to not over-populate
// HSTS state storage. Check top frame site for equality with site for cookies,
// don't store HSTS if it differs. IsolationInfo is not available everywhere,
// that's why we're using it only when parsing new HSTS state.
absl::optional<std::string> GetPartitionHashForAddingHSTS(
    const IsolationInfo& isolation_info) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return absl::nullopt;
  }

  // If the top frame scheme is secure and SiteForCookies doesn't match
  // TopFrameSite, then we don't want to store this HSTS state at all. Return an
  // empty hash in this case, which will be treated as a non-persistable
  // partition.
  if (IsTopFrameOriginCryptographic(isolation_info) &&
      isolation_info.site_for_cookies().site() !=
          *isolation_info.network_isolation_key().GetTopFrameSite()) {
    return std::string();
  }

  return GetHSTSPartitionHash(isolation_info.network_isolation_key());
}

absl::optional<std::string> GetPartitionHashForReadingHSTS(
    const NetworkIsolationKey& network_isolation_key) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return absl::nullopt;
  }
  return GetHSTSPartitionHash(network_isolation_key);
}

}  // namespace

bool TransportSecurityState::ShouldSSLErrorsBeFatal(
    const NetworkIsolationKey& network_isolation_key,
    const std::string& host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetPartitionHash(
      GetPartitionHashForReadingHSTS(network_isolation_key));
  return ShouldSSLErrorsBeFatal(host);
}

bool TransportSecurityState::ShouldUpgradeToSSL(
    const NetworkIsolationKey& network_isolation_key,
    const std::string& host,
    const NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetPartitionHash(
      GetPartitionHashForReadingHSTS(network_isolation_key));
  return ShouldUpgradeToSSL(host, net_log);
}

bool TransportSecurityState::AddHSTSHeader(const IsolationInfo& isolation_info,
                                           const std::string& host,
                                           const std::string& value) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetPartitionHash(
      GetPartitionHashForAddingHSTS(isolation_info));
  return AddHSTSHeader(host, value);
}

}  // namespace net
