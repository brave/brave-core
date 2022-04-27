/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_HTTP_TRANSPORT_SECURITY_STATE_H_
#define BRAVE_CHROMIUM_SRC_NET_HTTP_TRANSPORT_SECURITY_STATE_H_

#include "brave/net/http/partitioned_host_state_map.h"
#include "net/base/isolation_info.h"

#define enabled_sts_hosts_                                                  \
  enabled_sts_hosts_unused_;                                                \
                                                                            \
 public:                                                                    \
  bool ShouldSSLErrorsBeFatal(                                              \
      const NetworkIsolationKey& network_isolation_key,                     \
      const std::string& host);                                             \
  bool ShouldUpgradeToSSL(const NetworkIsolationKey& network_isolation_key, \
                          const std::string& host,                          \
                          const NetLogWithSource& net_log);                 \
  bool AddHSTSHeader(const IsolationInfo& isolation_info,                   \
                     const std::string& host, const std::string& value);    \
                                                                            \
 private:                                                                   \
  PartitionedHostStateMap<STSStateMap> enabled_sts_hosts_

#include "src/net/http/transport_security_state.h"

#undef enabled_sts_hosts_

#endif  // BRAVE_CHROMIUM_SRC_NET_HTTP_TRANSPORT_SECURITY_STATE_H_
