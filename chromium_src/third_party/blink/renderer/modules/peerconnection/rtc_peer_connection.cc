/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/messaging/message_port.h"
#include "third_party/blink/renderer/platform/instrumentation/instance_counters.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

#define IncrementCounter(...)                                              \
  IncrementCounter(__VA_ARGS__);                                           \
  if (RuntimeEnabledFeatures::BraveIsInTorContextEnabled()) {              \
    exception_state.ThrowDOMException(DOMExceptionCode::kNotAllowedError,  \
                                      "RTCPeerConnection is not allowed"); \
    return;                                                                \
  }

#include <third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.cc>

#undef IncrementCounter
