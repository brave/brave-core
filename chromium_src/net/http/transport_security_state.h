/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_HTTP_TRANSPORT_SECURITY_STATE_H_
#define BRAVE_CHROMIUM_SRC_NET_HTTP_TRANSPORT_SECURITY_STATE_H_

#include "brave/net/http/partitioned_host_state_map.h"
#include "net/base/isolation_info.h"

namespace net {
class TransportSecurityState;
using TransportSecurityState_BraveImpl = TransportSecurityState;
}  // namespace net

#define TransportSecurityState TransportSecurityState_ChromiumImpl

#define enabled_sts_hosts_                 \
  enabled_sts_hosts_unused_;               \
  friend TransportSecurityState_BraveImpl; \
  PartitionedHostStateMap<STSStateMap> enabled_sts_hosts_

#include "src/net/http/transport_security_state.h"  // IWYU pragma: export

#undef enabled_sts_hosts_
#undef TransportSecurityState

namespace net {

class NET_EXPORT TransportSecurityState
    : public TransportSecurityState_ChromiumImpl {
 public:
  using TransportSecurityState_ChromiumImpl::
      TransportSecurityState_ChromiumImpl;

  SSLUpgradeDecision GetSSLUpgradeDecision(
      const NetworkAnonymizationKey& network_anonymization_key,
      const std::string& host,
      const NetLogWithSource& net_log = NetLogWithSource());
  bool ShouldSSLErrorsBeFatal(
      const NetworkAnonymizationKey& network_anonymization_key,
      const std::string& host);
  bool ShouldUpgradeToSSL(
      const NetworkAnonymizationKey& network_anonymization_key,
      const std::string& host,
      const NetLogWithSource& net_log = NetLogWithSource());
  bool AddHSTSHeader(const IsolationInfo& isolation_info,
                     std::string_view host,
                     std::string_view value);

  // This is used only for manual adding via net-internals page.
  void AddHSTS(std::string_view host,
               const base::Time& expiry,
               bool include_subdomains);
  // These are used in some places where no NIK is available.
  bool ShouldSSLErrorsBeFatal(const std::string& host);
  bool ShouldUpgradeToSSL(const std::string& host,
                          const NetLogWithSource& net_log = NetLogWithSource());
  bool GetDynamicSTSState(const std::string& host, STSState* result);
  bool DeleteDynamicDataForHost(const std::string& host);
};

}  // namespace net

#endif  // BRAVE_CHROMIUM_SRC_NET_HTTP_TRANSPORT_SECURITY_STATE_H_
