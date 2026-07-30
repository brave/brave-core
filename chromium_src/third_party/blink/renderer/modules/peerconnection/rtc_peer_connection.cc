/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/messaging/message_port.h"
#include "third_party/blink/renderer/platform/instrumentation/instance_counters.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {
namespace {

// chrome-untrusted://aichat-code-sandbox executes untrusted AI-generated code
// in a sandboxed iframe which runs with an opaque origin. CSP cannot block
// WebRTC (ICE uses UDP), so block RTCPeerConnection for the page's entire
// frame tree by checking the local frame root's origin.
// Keep the host in sync with kAIChatCodeSandboxUIHost in
// brave/common/webui_url_constants.h.
bool IsInAIChatCodeSandbox(LocalDOMWindow* window) {
  if (!window || !window->GetFrame()) {
    return false;
  }
  LocalDOMWindow* root_window =
      window->GetFrame()->LocalFrameRoot().DomWindow();
  const SecurityOrigin* root_origin =
      root_window ? root_window->GetSecurityOrigin() : nullptr;
  return root_origin && root_origin->Protocol() == "chrome-untrusted" &&
         root_origin->Host() == "aichat-code-sandbox";
}

}  // namespace
}  // namespace blink

#define IncrementCounter(...)                                              \
  IncrementCounter(__VA_ARGS__);                                           \
  if (RuntimeEnabledFeatures::BraveIsInTorContextEnabled() ||              \
      IsInAIChatCodeSandbox(window)) {                                     \
    exception_state.ThrowDOMException(DOMExceptionCode::kNotAllowedError,  \
                                      "RTCPeerConnection is not allowed"); \
    return;                                                                \
  }

#include <third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.cc>

#undef IncrementCounter
