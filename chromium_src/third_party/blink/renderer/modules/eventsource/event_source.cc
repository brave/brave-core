/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/eventsource/event_source.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/scheme_registry.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/core/streams/readable_byte_stream_controller.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"

#define ConnectTimerFired(...) ConnectTimerFired_ChromiumImpl(__VA_ARGS__)
#define close() close_ChromiumImpl()
#define DidFail DidFail_ChromiumImpl
#define DidFailRedirectCheck DidFailRedirectCheck_ChromiumImpl

#include "src/third_party/blink/renderer/modules/eventsource/event_source.cc"

#undef ConnectTimerFired
#undef close
#undef DidFail
#undef DidFailRedirectCheck

namespace blink {

void EventSource::ConnectTimerFired(TimerBase* timer_base) {
  BraveConnect();
}

void EventSource::BraveConnect() {
  if (base::FeatureList::IsEnabled(blink::features::kRestrictEventSourcePool)) {
    ExecutionContext* execution_context = GetExecutionContext();
    const bool is_extension = CommonSchemeRegistry::IsExtensionScheme(
        execution_context->GetSecurityOrigin()->Protocol().Ascii());
    if (!is_extension &&
        brave::GetBraveFarblingLevelFor(
            execution_context,
            ContentSettingsType::BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL,
            BraveFarblingLevel::OFF) != BraveFarblingLevel::OFF) {
      event_source_in_use_tracker_ =
          ResourcePoolLimiter::GetInstance().IssueResourceInUseTracker(
              execution_context,
              ResourcePoolLimiter::ResourceType::kEventSource);
      if (!event_source_in_use_tracker_) {
        AbortConnectionAttempt();
        return;
      }
    }
  }
  Connect();
}

void EventSource::MaybeResetEventSourceInUseTracker() {
  if (base::FeatureList::IsEnabled(blink::features::kRestrictEventSourcePool)) {
    if (event_source_in_use_tracker_ != nullptr) {
      event_source_in_use_tracker_.reset();
    }
  }
}

void EventSource::close() {
  close_ChromiumImpl();
  MaybeResetEventSourceInUseTracker();
}

void EventSource::DidFail(uint64_t identifier, const ResourceError& error) {
  DidFail_ChromiumImpl(identifier, error);
  if (state_ == kClosed) {
    MaybeResetEventSourceInUseTracker();
  }
}

void EventSource::DidFailRedirectCheck(uint64_t identifier) {
  DidFailRedirectCheck_ChromiumImpl(identifier);
  if (state_ == kClosed) {
    MaybeResetEventSourceInUseTracker();
  }
}

}  // namespace blink
