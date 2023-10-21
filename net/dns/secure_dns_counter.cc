/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/dns/secure_dns_counter.h"

#include "base/no_destructor.h"

namespace net {

SecureDnsCounter::SecureDnsCounter() = default;

SecureDnsCounter* SecureDnsCounter::GetInstance() {
  static base::NoDestructor<SecureDnsCounter> secure_dns_counter;
  return secure_dns_counter.get();
}

void SecureDnsCounter::RecordAutoSecureTaskCount(
    int task_type_int,
    const DnsQueryTypeSet& query_types) {
  if (task_type_int < 0 ||
      task_type_int > static_cast<int>(DnsTaskType::kMaxValue)) {
    return;
  }

  DnsTaskType task_type = static_cast<DnsTaskType>(task_type_int);

  if (!query_types.Has(DnsQueryType::HTTPS)) {
    // Only report dns tasks that are requesting HTTPS records.
    return;
  }

  bool is_upgraded_task = task_type == DnsTaskType::SECURE_DNS;

  counters_lock_.Acquire();
  counts_.total_count++;
  if (is_upgraded_task) {
    counts_.upgraded_count++;
  }
  counters_lock_.Release();
}

DnsRequestCounts SecureDnsCounter::GetCountsAndReset() {
  DnsRequestCounts result;

  counters_lock_.Acquire();
  result = counts_;
  counts_ = DnsRequestCounts();
  counters_lock_.Release();

  return result;
}

}  // namespace net
