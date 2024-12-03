/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/websockets/websocket_channel_impl.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/scheme_registry.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

#define WebSocketChannelImpl WebSocketChannelImpl_ChromiumImpl

#include "src/third_party/blink/renderer/modules/websockets/websocket_channel_impl.cc"

#undef WebSocketChannelImpl

namespace blink {

// static
WebSocketChannelImpl* WebSocketChannelImpl::Create(
    ExecutionContext* execution_context,
    WebSocketChannelClient* client,
    std::unique_ptr<SourceLocation> location) {
  auto* channel = MakeGarbageCollected<WebSocketChannelImpl>(
      execution_context, client, std::move(location));
  channel->handshake_throttle_ =
      channel->GetBaseFetchContext()->CreateWebSocketHandshakeThrottle();
  return channel;
}

void WebSocketChannelImpl::TearDownFailedConnection() {
  if (base::FeatureList::IsEnabled(blink::features::kRestrictWebSocketsPool)) {
    websocket_in_use_tracker_.reset();
  }
  WebSocketChannelImpl_ChromiumImpl::TearDownFailedConnection();
}

bool WebSocketChannelImpl::ShouldDisallowConnection(const KURL& url) {
  if (base::FeatureList::IsEnabled(blink::features::kRestrictWebSocketsPool)) {
    const bool is_extension = CommonSchemeRegistry::IsExtensionScheme(
        execution_context_->GetSecurityOrigin()->Protocol().Ascii());
    if (!is_extension &&
        brave::GetBraveFarblingLevelFor(
            execution_context_,
            ContentSettingsType::BRAVE_WEBCOMPAT_WEB_SOCKETS_POOL,
            BraveFarblingLevel::OFF) != BraveFarblingLevel::OFF) {
      websocket_in_use_tracker_ =
          ResourcePoolLimiter::GetInstance().IssueResourceInUseTracker(
              execution_context_,
              ResourcePoolLimiter::ResourceType::kWebSocket);
      if (!websocket_in_use_tracker_) {
        return true;
      }
    }
  }
  return WebSocketChannelImpl_ChromiumImpl::ShouldDisallowConnection(url);
}

void WebSocketChannelImpl::Dispose() {
  if (base::FeatureList::IsEnabled(blink::features::kRestrictWebSocketsPool)) {
    websocket_in_use_tracker_.reset();
  }
  WebSocketChannelImpl_ChromiumImpl::Dispose();
}

}  // namespace blink
