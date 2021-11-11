/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin_hash.h"

// Ephemeral origin channel name altering is applied only to frame-based
// ExecutionContexts. This is fine because any Worker-based context still
// wouldn't be able to communicate with a frame in both directions because a
// frame-based BroadcastChannel will use an ephemeral origin instead of the one
// the worker is using.
#define GetRemoteNavigationAssociatedInterfaces                          \
  should_send_resource_timing_info_to_parent(); /* no-op */              \
  if (auto* origin = GetEphemeralStorageOrigin(window)) {                \
    name_ = name_ + String::Number(SecurityOriginHash::GetHash(origin)); \
  }                                                                      \
  frame->GetRemoteNavigationAssociatedInterfaces

#include "../../../../../../../third_party/blink/renderer/modules/broadcastchannel/broadcast_channel.cc"

#undef GetRemoteNavigationAssociatedInterfaces
