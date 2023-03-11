/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_EVENTSOURCE_EVENT_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_EVENTSOURCE_EVENT_SOURCE_H_

#include "brave/third_party/blink/renderer/core/resource_pool_limiter/resource_pool_limiter.h"

// Avoid redefining tokens in these headers:
#include "third_party/blink/renderer/core/html/closewatcher/close_watcher.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

#define ConnectTimerFired        \
  ConnectTimerFired(TimerBase*); \
  void ConnectTimerFired_ChromiumImpl

#define Connect \
  Connect();    \
  void BraveConnect

#define close \
  close();    \
  void close_ChromiumImpl

#define DidFail(...)                 \
  DidFail_ChromiumImpl(__VA_ARGS__); \
  void DidFail(__VA_ARGS__)

#define DidFailRedirectCheck(...)                      \
  MaybeResetEventSourceInUseTracker();                 \
  void DidFailRedirectCheck_ChromiumImpl(__VA_ARGS__); \
  void DidFailRedirectCheck(__VA_ARGS__)

#define world_                                               \
  world_;                                                    \
  std::unique_ptr<ResourcePoolLimiter::ResourceInUseTracker> \
      event_source_in_use_tracker_

#include "src/third_party/blink/renderer/modules/eventsource/event_source.h"  // IWYU pragma: export

#undef ConnectTimerFired
#undef Connect
#undef close
#undef DidFail
#undef DidFailRedirectCheck
#undef world_

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_EVENTSOURCE_EVENT_SOURCE_H_
