/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/dns/secure_dns_counter.h"

#define BRAVE_RECORD_DNS_COUNTS                                 \
  if (task_type.has_value()) {                                  \
    SecureDnsCounter::GetInstance()->RecordAutoSecureTaskCount( \
        static_cast<int>(*task_type), key_.query_types);        \
  }

#include "src/net/dns/host_resolver_manager.cc"

#undef BRAVE_RECORD_DNS_COUNTS
