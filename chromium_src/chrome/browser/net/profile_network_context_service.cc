/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
static const char* kBraveCTExcludedHosts[] = {
    // Critical endpoints that shouldn't require SCTs so they always work
    // TODO(djandries): remove laptop-updates once
    // https://github.com/brave/brave-browser/issues/16374 hits release
    "laptop-updates.brave.com",
    "updates.bravesoftware.com",
    "updates-cdn.bravesoftware.com",
    "usage-ping.brave.com",
    // Test host for manual testing
    "sct-exempted.bravesoftware.com",
};

#define BRAVE_PROFILE_NETWORK_CONTEXT_SERVICE_GET_CT_POLICY \
  for (const auto* host : kBraveCTExcludedHosts) {          \
    excluded.push_back(host);                               \
  }

#include "src/chrome/browser/net/profile_network_context_service.cc"
#undef BRAVE_PROFILE_NETWORK_CONTEXT_SERVICE_GET_CT_POLICY
