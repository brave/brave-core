/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#include "build/build_config.h"
#include "net/base/network_isolation_key.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/url_util.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

#define BRAVE_ENABLE_STATIC_PINS \
  enable_static_pins_ = true;    \
  enable_static_expect_ct_ = true;

#else

// Leave static pins disabled on Android and iOS, like upstream.
#define BRAVE_ENABLE_STATIC_PINS \
  {}

#endif

#define TransportSecurityState TransportSecurityState_ChromiumImpl

#include "src/net/http/transport_security_state.cc"

#undef BRAVE_ENABLE_STATIC_PINS

#undef TransportSecurityState

namespace net {

// Non-anonymous helper to friend with SchemefulSite.
class HSTSPartitionHashHelper {
 public:
  static std::string GetPartitionDomain(const SchemefulSite& schemeful_site) {
    DCHECK(base::FeatureList::IsEnabled(features::kBravePartitionHSTS));
    if (schemeful_site.has_registrable_domain_or_host()) {
      return schemeful_site.registrable_domain_or_host();
    }

    if (schemeful_site.site_as_origin_.opaque()) {
      std::string precursor_etld1_host =
          registry_controlled_domains::GetDomainAndRegistry(
              schemeful_site.site_as_origin_.GetTupleOrPrecursorTupleIfOpaque()
                  .host(),
              registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
      if (!precursor_etld1_host.empty()) {
        return precursor_etld1_host;
      }
    }

    return std::string();
  }
};

namespace {

bool IsTopFrameOriginCryptographic(const IsolationInfo& isolation_info) {
  return isolation_info.top_frame_origin() &&
         GURL::SchemeIsCryptographic(
             isolation_info.top_frame_origin()->scheme());
}

std::string GetHSTSPartitionHash(
    const NetworkIsolationKey& network_isolation_key) {
  DCHECK(base::FeatureList::IsEnabled(features::kBravePartitionHSTS));
  // An empty top frame site cannot be used as a partition key, return an empty
  // hash which will be treated as a non-persistable partition.
  if (!network_isolation_key.GetTopFrameSite().has_value()) {
    return std::string();
  }

  const std::string partition_domain =
      HSTSPartitionHashHelper::GetPartitionDomain(
          *network_isolation_key.GetTopFrameSite());
  if (partition_domain.empty()) {
    return std::string();
  }

  const std::string canonicalized_partition_domain =
      CanonicalizeHost(partition_domain);
  if (canonicalized_partition_domain.empty()) {
    return std::string();
  }

  return HashHost(canonicalized_partition_domain);
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

// Use NetworkIsolationKey to create PartitionHash for accessing/storing data.
absl::optional<std::string> GetPartitionHashForHSTS(
    const NetworkIsolationKey& network_isolation_key) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return absl::nullopt;
  }
  return GetHSTSPartitionHash(network_isolation_key);
}

// Use host-bound NetworkIsolationKey in cases when no NetworkIsolationKey is
// available. Such cases may include net-internals page, PasswordManager.
// All network::NetworkContext HSTS-related public methods will use this.
absl::optional<std::string> GetHostBoundPartitionHashForHSTS(
    const std::string& host) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return absl::nullopt;
  }
  SchemefulSite schemeful_site(url::Origin::Create(GURL("https://" + host)));
  NetworkIsolationKey network_isolation_key(schemeful_site, schemeful_site);
  return GetHSTSPartitionHash(network_isolation_key);
}

}  // namespace

bool TransportSecurityState::ShouldSSLErrorsBeFatal(
    const NetworkIsolationKey& network_isolation_key,
    const std::string& host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetPartitionHashForHSTS(network_isolation_key));
  return TransportSecurityState_ChromiumImpl::ShouldSSLErrorsBeFatal(host);
}

bool TransportSecurityState::ShouldUpgradeToSSL(
    const NetworkIsolationKey& network_isolation_key,
    const std::string& host,
    const NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetPartitionHashForHSTS(network_isolation_key));
  return TransportSecurityState_ChromiumImpl::ShouldUpgradeToSSL(host, net_log);
}

bool TransportSecurityState::AddHSTSHeader(const IsolationInfo& isolation_info,
                                           const std::string& host,
                                           const std::string& value) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetPartitionHashForAddingHSTS(isolation_info));
  if (enabled_sts_hosts_.HasPartitionHash() &&
      !enabled_sts_hosts_.IsPartitionHashValid()) {
    return false;
  }
  return TransportSecurityState_ChromiumImpl::AddHSTSHeader(host, value);
}

void TransportSecurityState::AddHSTS(const std::string& host,
                                     const base::Time& expiry,
                                     bool include_subdomains) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetHostBoundPartitionHashForHSTS(host));
  TransportSecurityState_ChromiumImpl::AddHSTS(host, expiry,
                                               include_subdomains);
}

bool TransportSecurityState::ShouldSSLErrorsBeFatal(const std::string& host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetHostBoundPartitionHashForHSTS(host));
  return TransportSecurityState_ChromiumImpl::ShouldSSLErrorsBeFatal(host);
}

bool TransportSecurityState::ShouldUpgradeToSSL(
    const std::string& host,
    const NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetHostBoundPartitionHashForHSTS(host));
  return TransportSecurityState_ChromiumImpl::ShouldUpgradeToSSL(host, net_log);
}

bool TransportSecurityState::GetDynamicSTSState(const std::string& host,
                                                STSState* result) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetHostBoundPartitionHashForHSTS(host));
  return TransportSecurityState_ChromiumImpl::GetDynamicSTSState(host, result);
}

bool TransportSecurityState::DeleteDynamicDataForHost(const std::string& host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  const bool chromium_deleted =
      TransportSecurityState_ChromiumImpl::DeleteDynamicDataForHost(host);

  bool brave_deleted = false;
  if (base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    const std::string canonicalized_host = CanonicalizeHost(host);
    if (!canonicalized_host.empty()) {
      if (enabled_sts_hosts_.DeleteDataInAllPartitions(
              HashHost(canonicalized_host))) {
        brave_deleted = true;
      }
    }
  }

  if (brave_deleted)
    DirtyNotify();
  return chromium_deleted || brave_deleted;
}

}  // namespace net
