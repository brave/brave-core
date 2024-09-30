/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#include <optional>

#include "base/strings/strcat.h"
#include "build/build_config.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/url_util.h"

#define BRAVE_ENABLE_STATIC_PINS enable_static_pins_ = true;

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

TransportSecurityState::HashedHost GetHSTSPartitionHash(
    const NetworkAnonymizationKey& network_anonymization_key) {
  DCHECK(base::FeatureList::IsEnabled(features::kBravePartitionHSTS));
  // An empty top frame site cannot be used as a partition key, return an empty
  // hash which will be treated as a non-persistable partition.
  if (!network_anonymization_key.GetTopFrameSite().has_value()) {
    return TransportSecurityState::HashedHost();
  }

  const std::string partition_domain =
      HSTSPartitionHashHelper::GetPartitionDomain(
          *network_anonymization_key.GetTopFrameSite());
  if (partition_domain.empty()) {
    return TransportSecurityState::HashedHost();
  }

  const std::vector<uint8_t> canonicalized_partition_domain =
      CanonicalizeHost(partition_domain);
  if (canonicalized_partition_domain.empty()) {
    return TransportSecurityState::HashedHost();
  }

  return HashHost(canonicalized_partition_domain);
}

// Use only top frame site as a key for HSTS partitioning to not over-populate
// HSTS state storage. Check top frame site for equality with site for cookies,
// don't store HSTS if it differs. IsolationInfo is not available everywhere,
// that's why we're using it only when parsing new HSTS state.
std::optional<TransportSecurityState::HashedHost> GetPartitionHashForAddingHSTS(
    const IsolationInfo& isolation_info) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return std::nullopt;
  }

  // If the top frame scheme is secure and SiteForCookies doesn't match
  // TopFrameSite, then we don't want to store this HSTS state at all. Return an
  // empty hash in this case, which will be treated as a non-persistable
  // partition.
  if (IsTopFrameOriginCryptographic(isolation_info) &&
      isolation_info.site_for_cookies().site() !=
          *isolation_info.network_anonymization_key().GetTopFrameSite()) {
    return TransportSecurityState::HashedHost();
  }

  return GetHSTSPartitionHash(isolation_info.network_anonymization_key());
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data.
std::optional<TransportSecurityState::HashedHost> GetPartitionHashForHSTS(
    const NetworkAnonymizationKey& network_anonymization_key) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return std::nullopt;
  }
  return GetHSTSPartitionHash(network_anonymization_key);
}

// Use host-bound NetworkAnonymizationKey in cases when no
// NetworkAnonymizationKey is available. Such cases may include net-internals
// page, PasswordManager. All network::NetworkContext HSTS-related public
// methods will use this.
std::optional<TransportSecurityState::HashedHost>
GetHostBoundPartitionHashForHSTS(std::string_view host) {
  if (!base::FeatureList::IsEnabled(features::kBravePartitionHSTS)) {
    return std::nullopt;
  }
  SchemefulSite schemeful_site(
      url::Origin::Create(GURL(base::StrCat({"https://", host}))));
  auto network_anonymization_key =
      net::NetworkAnonymizationKey::CreateFromFrameSite(schemeful_site,
                                                        schemeful_site);
  return GetHSTSPartitionHash(network_anonymization_key);
}

}  // namespace

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
SSLUpgradeDecision TransportSecurityState::GetSSLUpgradeDecision(
    const NetworkAnonymizationKey& network_anonymization_key,
    const std::string& host,
    const NetLogWithSource& net_log) {
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetPartitionHashForHSTS(network_anonymization_key));
  return TransportSecurityState_ChromiumImpl::GetSSLUpgradeDecision(host,
                                                                    net_log);
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
bool TransportSecurityState::ShouldSSLErrorsBeFatal(
    const NetworkAnonymizationKey& network_anonymization_key,
    const std::string& host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetPartitionHashForHSTS(network_anonymization_key));
  return TransportSecurityState_ChromiumImpl::ShouldSSLErrorsBeFatal(host);
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
bool TransportSecurityState::ShouldUpgradeToSSL(
    const NetworkAnonymizationKey& network_anonymization_key,
    const std::string& host,
    const NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetPartitionHashForHSTS(network_anonymization_key));
  return TransportSecurityState_ChromiumImpl::ShouldUpgradeToSSL(host, net_log);
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
bool TransportSecurityState::AddHSTSHeader(const IsolationInfo& isolation_info,
                                           std::string_view host,
                                           std::string_view value) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetPartitionHashForAddingHSTS(isolation_info));
  if (enabled_sts_hosts_.HasPartitionHash() &&
      !enabled_sts_hosts_.IsPartitionHashValid()) {
    return false;
  }
  return TransportSecurityState_ChromiumImpl::AddHSTSHeader(host, value);
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
void TransportSecurityState::AddHSTS(std::string_view host,
                                     const base::Time& expiry,
                                     bool include_subdomains) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetHostBoundPartitionHashForHSTS(host));
  TransportSecurityState_ChromiumImpl::AddHSTS(host, expiry,
                                               include_subdomains);
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
bool TransportSecurityState::ShouldSSLErrorsBeFatal(const std::string& host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetHostBoundPartitionHashForHSTS(host));
  return TransportSecurityState_ChromiumImpl::ShouldSSLErrorsBeFatal(host);
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
bool TransportSecurityState::ShouldUpgradeToSSL(
    const std::string& host,
    const NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto auto_reset_partition_hash = enabled_sts_hosts_.SetScopedPartitionHash(
      GetHostBoundPartitionHashForHSTS(host));
  return TransportSecurityState_ChromiumImpl::ShouldUpgradeToSSL(host, net_log);
}

// Use NetworkAnonymizationKey to create PartitionHash for accessing/storing
// data before calling Chromium implementation
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
    const std::vector<uint8_t> canonicalized_host = CanonicalizeHost(host);
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
