/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_DNS_SECURE_DNS_COUNTER_H_
#define BRAVE_NET_DNS_SECURE_DNS_COUNTER_H_

#include "base/synchronization/lock.h"
#include "net/base/net_export.h"
#include "net/dns/public/dns_query_type.h"

namespace net {

struct DnsRequestCounts {
  size_t total_count = 0;
  size_t upgraded_count = 0;
};

enum class DnsTaskType {
  SYSTEM = 0,
  DNS = 1,
  SECURE_DNS = 2,
  kMaxValue = SECURE_DNS
};

// Counts total and secure DNS requests for P3A purposes.
// misc_metrics::DohMetrics will query GetCountsAndReset
// on a fixed interval to retrieve the counts. Two
// counts (total and upgraded requests) are maintained;
// locking is used to ensure that count updates are atomic,
// and to handle concurrent count updates (from the HostResolverManager)
// & retrieval (from DohMetrics in the browser process via mojo).
class NET_EXPORT SecureDnsCounter {
 public:
  SecureDnsCounter();

  SecureDnsCounter(const SecureDnsCounter&) = delete;
  SecureDnsCounter& operator=(const SecureDnsCounter&) = delete;

  static SecureDnsCounter* GetInstance();

  void RecordAutoSecureTaskCount(int task_type_int,
                                 const DnsQueryTypeSet& query_types);

  DnsRequestCounts GetCountsAndReset();

 private:
  base::Lock counters_lock_;
  DnsRequestCounts counts_;
};

}  // namespace net

#endif  // BRAVE_NET_DNS_SECURE_DNS_COUNTER_H_
